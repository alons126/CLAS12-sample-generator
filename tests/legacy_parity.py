"""Compare new outputs to controlled archived reference runs.

Purpose:
    Check exact LUND bytes and numerical histograms while identifying the short-input correction.

Workflow:
    CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.

Notes:
    Test fixtures are isolated; protected external and legacy sources are read-only.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def uniform_output(root, channel, beam):
    """Return the resolved uniform run directory for one requested output root."""
    energy_mev = int(float(beam) * 1000 + 0.5)
    return root / f'Uniform_sample_{channel}_{energy_mev:04d}MeV'


# run --------------------------------------------------------------------
# region run
def run(*args):
    """Run a parity-test command with captured diagnostics.

    Algorithm:
        Execute the supplied argv list and require success.

    Args:
        args: Executable and arguments.

    Returns:
        Captured subprocess result; failure raises an assertion.
    """
    result = subprocess.run([str(a) for a in args], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return result
# endregion


# compare --------------------------------------------------------------------
# region compare
def compare(actual, expected):
    """Require byte-identical new and reference LUND files.

    Algorithm:
        Compare bytes; on disagreement identify the first differing line or line-count mismatch.

    Args:
        actual: New output path.
        expected: Archived-reference output path.

    Returns:
        None; mismatches raise an assertion with file context.
    """
    a, e = actual.read_bytes(), expected.read_bytes()
    if a != e:
        al, el = a.decode().splitlines(), e.decode().splitlines()
        for i, (x,y) in enumerate(zip(al,el),1):
            assert x == y, f'{actual.name}, line {i}: new={x!r}; legacy={y!r}'
        raise AssertionError(f'{actual}: new {len(al)} lines vs legacy {len(el)} lines')
# endregion


mode, current, legacy = sys.argv[1:4]
# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-parity-') as temp:
    root = Path(temp)
    if mode == 'uniform':
        # All production beam settings and channels at a small deterministic count.
        for beam in ['2.07052','4.02962','5.98636']:
            for channel in ['1e','ep','en','tester']:
                name=beam+'-'+channel
                original, new_root = root/(name+'-old'), root/(name+'-new')
                new = uniform_output(new_root, '1e' if channel == 'tester' else channel, beam)
                run(legacy,channel,original,beam,64,1,67890,12345,'Ar')
                extra = ['--electron-momentum','beam','--target','point'] if channel=='tester' else []
                run(current,'--channel','1e' if channel=='tester' else channel,'--beam-energy',beam,'--events',64,
                    '--seed',67890,'--vertex-seed',12345,'--output',new_root,*extra)
                run(sys.argv[4], original/'histograms.root', new/'legacy_histograms.root')
                m=json.loads((new/'manifest.json').read_text())
                assert len(m['files']) == 1
                compare(new/m['files'][0]['path'],original/'legacy_1.txt')
        for target in ['liquid','4-foil','1-foil','1-foil-small','1-foil-large','Ca']:
            original,new_root=root/(target+'-old'),root/(target+'-new')
            new=uniform_output(new_root,'1e','2.07052')
            run(legacy,'1e',original,2.07052,64,1,67890,12345,target)
            run(current,'--channel','1e','--beam-energy',2.07052,'--target',target,'--events',64,'--output',new_root)
            m=json.loads((new/'manifest.json').read_text())
            compare(new/m['files'][0]['path'],original/'legacy_1.txt')
    else:
        fixture=sys.argv[4]
        for beam,label,target,A,Z in [('2.07052','2070MeV','1-foil-small',12,6),('4.02962','4029MeV','1-foil-large',12,6),('5.98636','5986MeV','4-foil',12,6)]:
            gst=root/f'C12_GEM21_11a_00_000_{label}.root'
            run(fixture,gst,'parity')
            original,new=root/(label+'-old'),root/(label+'-new')
            run(legacy,gst,original,1,target,A,Z)
            run(current,'--input',gst,'--beam-energy',beam,'--target',target,'--A',A,'--Z',Z,'--events',10000,'--output',new)
            m=json.loads((new/'manifest.json').read_text())
            run(sys.argv[5], original/'histograms.root', new/'legacy_histograms.root')
            old=list((original/'lundfiles').glob('*.txt'))
            assert len(old)==1 and m['written_events']==10000
            compare(new/m['files'][0]['path'],old[0])
        # Expose rather than reproduce the archived early-termination defect.
        gst=root/'C12_GEM21_11a_00_000_2070MeV_short.root'
        run(fixture,gst)
        original,new=root/'short-old',root/'short-new'
        run(legacy,gst,original,1,'1-foil-small',12,6)
        run(current,'--input',gst,'--beam-energy',2.07052,'--target','1-foil-small','--A',12,'--Z',6,'--events',10000,'--output',new)
        old=next((original/'lundfiles').glob('*.txt')).read_text().splitlines()
        m=json.loads((new/'manifest.json').read_text())
        assert len(old)==8 and m['written_events']==6
        print('Confirmed intentional difference: legacy short-input truncation writes 1 event; new writes all 6 accepted events.')
print(mode+' legacy LUND parity passed')

# endregion
