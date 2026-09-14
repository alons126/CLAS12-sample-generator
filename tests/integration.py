"""End-to-end checks of emitted LUND data, not generator internals."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile


def run(*args, ok=True):
    result = subprocess.run([str(x) for x in args], capture_output=True, text=True)
    assert (result.returncode == 0) == ok, result.stdout + result.stderr
    return result


def read_run(directory):
    manifest = json.loads((directory / 'manifest.json').read_text())
    events = []
    for entry in manifest['files']:
        lines = iter((directory / entry['path']).read_text().splitlines())
        count = 0
        for line in lines:
            header = line.split()
            assert len(header) == 10
            particles = [list(map(float, next(lines).split())) for _ in range(int(header[0]))]
            for i, particle in enumerate(particles, 1):
                assert len(particle) == 14 and particle[0] == i and particle[2] == 1
                assert all(math.isfinite(x) for x in particle)
                p2 = sum(x*x for x in particle[6:9])
                assert math.isclose(particle[9]**2, p2 + particle[10]**2, rel_tol=1e-8, abs_tol=1e-9)
                assert particle[11:] == particles[0][11:]
            events.append((header, particles))
            count += 1
        assert count == entry['events']
    assert len(events) == manifest['written_events']
    assert (directory / 'monitoring.root').stat().st_size > 0
    return manifest, events


def angles(particle):
    x,y,z = particle[6:9]
    p = math.sqrt(x*x+y*y+z*z)
    return p, math.degrees(math.acos(z/p)), math.degrees(math.atan2(y,x))


mode, executable = sys.argv[1:3]
with tempfile.TemporaryDirectory(prefix='clas12-integration-') as temp:
    root = Path(temp)
    run(executable, '--help')
    if mode == 'uniform':
        for channel, pid in [('1e',11),('ep',2212),('en',2112)]:
            output = root / channel
            settings = ['--channel', channel, '--files', '2', '--events-per-file', '250', '--seed', '17', '--vertex-seed', '23']
            run(executable, *settings, '--output', output)
            manifest, events = read_run(output)
            assert [f['events'] for f in manifest['files']] == [250,250]
            assert [int(h[8]) for h,p in events] == list(range(500))
            for header, particles in events:
                assert len(particles) == (1 if channel == '1e' else 2)
                assert particles[-1][3] == pid
                assert -5.75 <= particles[0][13] <= -5.25
                p,theta,phi = angles(particles[-1])
                assert 5-1e-7 <= theta <= (40 if channel == '1e' else 45 if channel == 'ep' else 35)+1e-7
                if channel != '1e':
                    assert math.isclose(p,1,abs_tol=1e-8)
                    ep, et, ef = angles(particles[0])
                    assert math.isclose(ep,5.98636,abs_tol=1e-8) and math.isclose(et,25,abs_tol=1e-7)
                    assert abs(((ef-5+180)%60)-0) < 1e-6 or abs(((ef-5+180)%60)-60) < 1e-6
            # The default is flat in theta, not cos(theta); a broad mean check guards it.
            mean = sum(angles(p[-1])[1] for h,p in events)/len(events)
            hi = 40 if channel == '1e' else 45 if channel == 'ep' else 35
            assert abs(mean - (5+hi)/2) < 2
            second = root / (channel+'-repeat')
            run(executable, *settings, '--output', second)
            for file in manifest['files']:
                assert (output/file['path']).read_bytes() == (second/file['path']).read_bytes()
            sentinel = output/'keep.txt'
            sentinel.write_text('keep')
            run(executable, *settings, '--output', output, ok=False)
            assert sentinel.read_text() == 'keep'
        config = root/'sample.conf'
        config.write_text('# test precedence\nchannel = en\nevents-per-file = 3\nbeam-energy = 2.07052\n')
        run(executable, '--config', config, '--events-per-file', '5', '--output', root/'configured')
        m,_ = read_run(root/'configured')
        assert m['written_events'] == 5 and m['config']['trigger-phi-offset'] == '16'
        run(executable, '--channel', 'ep', '--nucleon-momentum', 'uniform', '--events-per-file', '100', '--output', root/'variable')
        _,events=read_run(root/'variable')
        momenta=[angles(p[-1])[0] for h,p in events]
        assert min(momenta)>=0.3 and max(momenta)<=5.98636 and max(momenta)-min(momenta)>1
        run(executable, '--electron-momentum', 'beam', '--target', 'point', '--events-per-file', '5', '--output', root/'tester')
        _,events=read_run(root/'tester')
        assert all(p[0][11:]==[0,0,0] and math.isclose(angles(p[0])[0],5.98636,abs_tol=1e-8) for h,p in events)
        for key,value in [('channel','bad'),('seed','0'),('files','0'),('target','missing'),('beam-energy','nan'),('electron-theta-min','50'),('A','0'),('unknown','1'),('prefix','../bad')]:
            output=root/('invalid-'+key)
            run(executable, '--'+key, value, '--output', output, ok=False)
            assert not output.exists()
    else:
        fixture = sys.argv[3]
        gst = root/'gst.root'
        run(fixture, gst)
        output = root/'converted'
        run(executable, '--input', gst, '--output', output, '--files', '3', '--events-per-file', '4', '--A','40','--Z','18')
        m,events=read_run(output)
        assert m['scanned_events']==7 and m['written_events']==6
        assert [f['events'] for f in m['files']]==[4,2]
        assert [int(h[9]) for h,p in events]==[1,2,3,4,1,1]
        assert [int(h[8]) for h,p in events]==[0,1,2,3,5,6]
        for h,p in events:
            assert h[1:4]==['40','18','7']
            assert [int(x[3]) for x in p]==[11,2212,2112,211,-211,111,22]
            assert p[0][6:9]==[0.5,0.1,2]
        run(executable,'--input',gst,'--output',root/'limited','--files','1','--events-per-file','2')
        m,_=read_run(root/'limited')
        assert m['written_events']==2 and m['scanned_events']==2 and len(m['files'])==1
        run(fixture,root/'missing.root','missing')
        run(executable,'--input',root/'missing.root','--output',root/'bad',ok=False)
        assert not (root/'bad').exists()
        run(executable,'--input',root/'absent.root','--output',root/'absent',ok=False)
        for kind in ['wrong-type','empty','unsupported']:
            run(fixture,root/(kind+'.root'),kind)
            bad=root/(kind+'-output')
            run(executable,'--input',root/(kind+'.root'),'--output',bad,ok=False)
            assert not (bad/'manifest.json').exists()
        run(fixture,root/'large.root','large')
        run(executable,'--input',root/'large.root','--output',root/'large-output','--events-per-file','4')
        m,e=read_run(root/'large-output')
        assert m['written_events']==4 and all(len(p)==7 for h,p in e)
        run(fixture,root/'chain-a.root')
        run(fixture,root/'chain-b.root','missing')
        run(executable,'--input',root/'chain-*.root','--output',root/'chain-output',ok=False)
        assert not (root/'chain-output/manifest.json').exists()

print(mode+' integration passed')
