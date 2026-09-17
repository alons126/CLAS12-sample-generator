#
# Created by Alon Sportes on 14/09/2026.
#

"""Check generator and converter output contracts.

Purpose:
    Inspect emitted LUND records, manifests, deterministic seeds and rejected inputs.

Workflow:
    CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.

Notes:
    Test fixtures are isolated; protected external and legacy sources are read-only.
"""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile


# run --------------------------------------------------------------------
# region run
def run(*args, ok=True):
    """Run a test command and check its expected status.

    Algorithm:
        Enable precise output for normal generator calls, capture diagnostics, and assert the expected result.

    Args:
        args: Executable and arguments.
        ok: Whether successful exit is expected.

    Returns:
        Captured subprocess result.
    """
    args = (*args, "--lund-format", "precise") if str(args[0]) == executable and "--help" not in args else args
    result = subprocess.run([str(x) for x in args], capture_output=True, text=True)
    assert (result.returncode == 0) == ok, result.stdout + result.stderr
    return result
# endregion


# read_run --------------------------------------------------------------------
# region read_run
def read_run(directory):
    """Read a completed run and verify LUND structural invariants.

    Algorithm:
        Read the manifest, parse every header and particle record, and check counts and particle energies.

    Args:
        directory: Generated run directory.

    Returns:
        Manifest and parsed event collection; malformed output raises an assertion.
    """
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
# endregion


def physical_output(root, beam='5.98636'):
    """Return the default metadata-derived physical run directory."""
    q2 = {'2.07052':'Q2_0_02', '4.02962':'Q2_0_25', '5.98636':'Q2_0_40'}.get(str(beam), 'none')
    mev = int(float(beam) * 1000 + 0.5)
    return root/f'rgm_fall2021_Ar__genie-unknown__unknown__{q2}__{mev}MeV_GEMC-unknown'


# angles --------------------------------------------------------------------
# region angles
def angles(particle):
    """Recover polar and azimuthal angles from a particle record.

    Algorithm:
        Convert the Cartesian momentum to degrees using polar and azimuthal formulas.

    Args:
        particle: Parsed fourteen-field LUND particle record.

    Returns:
        Polar and azimuthal angles in degrees.
    """
    x,y,z = particle[6:9]
    p = math.sqrt(x*x+y*y+z*z)
    return p, math.degrees(math.acos(z/p)), math.degrees(math.atan2(y,x))
# endregion


mode, executable = sys.argv[1:3]
# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-integration-') as temp:
    root = Path(temp)
    run(executable, '--help')
    if mode == 'uniform':
        for channel, pid in [('1e',11),('ep',2212),('en',2112)]:
            output_root = root / channel
            output = output_root / f'Uniform_sample_{channel}_5986MeV'
            settings = ['--channel', channel, '--events', '10001', '--seed', '17', '--vertex-seed', '23']
            run(executable, *settings, '--output', output_root)
            manifest, events = read_run(output)
            assert [f['events'] for f in manifest['files']] == [10000,1]
            assert [int(h[8]) for h,p in events] == list(range(10001))
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
            second = root / (channel+'-repeat') / f'Uniform_sample_{channel}_5986MeV'
            run(executable, *settings, '--output', second.parent)
            for file in manifest['files']:
                assert (output/file['path']).read_bytes() == (second/file['path']).read_bytes()
            sentinel = output/'keep.txt'
            sentinel.write_text('keep')
            rerun = run(executable, *settings, '--output', output_root)
            assert 'Replacing existing run directory (legacy behavior):' in rerun.stdout
            assert not sentinel.exists()
        config = root/'sample.conf'
        config.write_text('# test precedence\nchannel = en\nevents = 3\nbeam-energy = 2.07052\n')
        configured = root/'configured'/ 'Uniform_sample_en_2070MeV'
        run(executable, '--config', config, '--events', '5', '--output', configured.parent)
        m,_ = read_run(configured)
        assert m['written_events'] == 5 and m['config']['trigger-phi-offset'] == '16'
        variable = root/'variable'/ 'Uniform_sample_ep_5986MeV'
        run(executable, '--channel', 'ep', '--nucleon-momentum', 'uniform', '--events', '100', '--output', variable.parent)
        _,events=read_run(variable)
        momenta=[angles(p[-1])[0] for h,p in events]
        assert min(momenta)>=0.3 and max(momenta)<=5.98636 and max(momenta)-min(momenta)>1
        tester = root/'tester'/ 'Uniform_sample_1e_5986MeV'
        run(executable, '--electron-momentum', 'beam', '--target', 'point', '--events', '5', '--output', tester.parent)
        _,events=read_run(tester)
        assert all(p[0][11:]==[0,0,-3] and math.isclose(angles(p[0])[0],5.98636,abs_tol=1e-8) for h,p in events)
        # Every material-bearing RG-M target resolves nuclear metadata and one external geometry key.
        targets = {
            'H1':(1,1,'liquid'), 'D2':(2,1,'liquid'), 'He4':(4,2,'liquid'),
            'C12-four-foil':(12,6,'4-foil'), 'Sn-nat-four-foil':(119,50,'4-foil'),
            'Ca40':(40,20,'Ca'), 'Ca48':(48,20,'Ca'), 'C12-small':(12,6,'1-foil-small'),
            'C12-large':(12,6,'1-foil-large'), 'Ar40':(40,18,'Ar'),
            'Sn120-large':(120,50,'1-foil-large'), 'C12-legacy':(12,6,'1-foil'),
            'Sn120-legacy':(120,50,'1-foil'),
        }
        for name,(A,Z,geometry) in targets.items():
            parent=root/'targets'/name
            run(executable,'--rgm-target',name,'--events','1','--output',parent)
            manifest,_=read_run(parent/'Uniform_sample_1e_5986MeV')
            assert manifest['config']['A']==str(A) and manifest['config']['Z']==str(Z)
            assert manifest['config']['target']==geometry
        for key,value in [('channel','bad'),('seed','0'),('files','0'),('target','missing'),('beam-energy','nan'),('electron-theta-min','50'),('A','0'),('unknown','1'),('prefix','../bad')]:
            output=root/('invalid-'+key)
            run(executable, '--'+key, value, '--output', output, ok=False)
            assert not output.exists()
    else:
        fixture = sys.argv[3]
        gst = root/'gst.root'
        run(fixture, gst)
        output_root = root/'converted'; output = physical_output(output_root)
        run(executable, '--input', gst, '--output', output_root, '--events', '6', '--A','40','--Z','18')
        m,events=read_run(output)
        assert m['workflow']=='physical' and m['config']['event-generator']=='genie'
        assert m['scanned_events']==7 and m['written_events']==6
        assert [f['events'] for f in m['files']]==[6]
        assert [int(h[9]) for h,p in events]==[1,2,3,4,1,1]
        assert [int(h[8]) for h,p in events]==[0,1,2,3,5,6]
        for h,p in events:
            assert h[1:4]==['40','18','7']
            assert [int(x[3]) for x in p]==[11,2212,2112,211,-211,111,22]
            assert p[0][6:9]==[0.5,0.1,2]
        named_root=root/'named'
        run(executable,'--input',gst,'--output',named_root,'--events','1','--rgm-target','C12-small',
            '--event-generator-version','3.2.2','--tune','GEM21_11a_00_000','--q2-cut','Q2_0_40','--gemc-version','5.14')
        named=named_root/'rgm_fall2021_C_S__genie-3.2.2__GEM21_11a_00_000__Q2_0_40__5986MeV_GEMC-5.14'
        nm,_=read_run(named)
        assert nm['config']['A']=='12' and nm['config']['Z']=='6' and nm['config']['target']=='1-foil-small'
        limited_root=root/'limited'
        run(executable,'--input',gst,'--output',limited_root,'--events','2')
        m,_=read_run(physical_output(limited_root))
        assert m['written_events']==2 and m['scanned_events']==2 and len(m['files'])==1
        run(fixture,root/'missing.root','missing')
        run(executable,'--input',root/'missing.root','--output',root/'bad',ok=False)
        run(executable,'--input',root/'absent.root','--output',root/'absent',ok=False)
        for kind in ['wrong-type','empty','unsupported']:
            run(fixture,root/(kind+'.root'),kind)
            bad=root/(kind+'-output')
            run(executable,'--input',root/(kind+'.root'),'--output',bad,ok=False)
            assert not (physical_output(bad)/'manifest.json').exists()
        run(fixture,root/'large.root','large')
        large_root=root/'large-output'
        run(executable,'--input',root/'large.root','--output',large_root,'--events','4')
        m,e=read_run(physical_output(large_root))
        assert m['written_events']==4 and all(len(p)==7 for h,p in e)
        run(fixture,root/'chain-a.root')
        run(fixture,root/'chain-b.root','missing')
        run(executable,'--input',root/'chain-*.root','--output',root/'chain-output',ok=False)
        assert not (physical_output(root/'chain-output')/'manifest.json').exists()

print(mode+' integration passed')

# endregion
