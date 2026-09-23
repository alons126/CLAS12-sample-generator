#
# Created by Alon Sportes on 14/09/2026.
#

"""Compare new outputs to controlled archived reference runs.

Purpose:
    Check exact LUND bytes, numerical histograms, and the physical-input cutoff during development.

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

    energy_mev = {'2.07052':2070, '4.02962':4029, '5.98636':5986}.get(str(beam), int(float(beam) * 1000 + 0.5))

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
def comparable_lines(path, drop_pi0=False):
    """Return tokenized LUND lines with an optional legacy-pi0 normalization.

    Algorithm:
        Parse complete events, remove legacy PDG 111 particle records when requested, and update the
        corresponding header multiplicity to match the maintained upstream-decay contract.

    Args:
        path: LUND file to read.
        drop_pi0: Whether to remove archived neutral-pion output records.

    Returns:
        Token lists in original event and retained-particle order.
    """

    source = [line.split() for line in path.read_text().splitlines()]

    if not drop_pi0:
        return source

    result = []
    offset = 0

    while offset < len(source):
        header = source[offset]
        count = int(header[0])
        particles = source[offset + 1:offset + 1 + count]
        retained = [particle for particle in particles if int(particle[3]) != 111]
        for index, particle in enumerate(retained, 1):
            particle[0] = str(index)
        result.append([str(len(retained)), *header[1:]])
        result.extend(retained)
        offset += count + 1

    assert offset == len(source), f'{path}: incomplete LUND event'

    return result

def compare(actual, expected, ignore_vertex=False, drop_legacy_pi0=False, compare_mass_energy=False):
    """Compare archived and maintained LUND records under selected normalization rules.

    Algorithm:
        Compare headers and selected particle fields, optionally including energy/mass or omitting
        tester vertices.
        Physical parity may also remove archived neutral pions, which the maintained converter requires
        GENIE to decay upstream into photons.

    Args:
        actual: New output path.
        expected: Archived-reference output path.
        ignore_vertex: Whether to omit vertex columns from the comparison.
        drop_legacy_pi0: Whether to normalize away archived PDG 111 records and renumber particles.
        compare_mass_energy: Whether energy and mass must also match exactly.

    Returns:
        None; mismatches raise an assertion with file context.
    """

    al = comparable_lines(actual)
    el = comparable_lines(expected, drop_legacy_pi0)
    assert len(al) == len(el), f'{actual}: new {len(al)} lines vs legacy {len(el)} lines'
    remaining = 0

    for i, (a, e) in enumerate(zip(al, el), 1):
        av, ev = a, e

        if remaining == 0:
            assert av == ev, f'{actual.name}, header line {i}: new={a!r}; legacy={e!r}'
            remaining = int(av[0])
        else:
            keep = list(range(9)) + ([] if ignore_vertex else [11, 12, 13])
            if compare_mass_energy:
                keep.extend([9, 10])
            assert [av[j] for j in keep] == [ev[j] for j in keep], f'{actual.name}, particle line {i}: new={a!r}; legacy={e!r}'
            remaining -= 1
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
                label = 'electron-tester' if channel == 'tester' else {'ep':'epFD','en':'enFD'}.get(channel,channel)
                new = uniform_output(new_root, label, beam)

                run(legacy,channel,original,beam,64,1,67890,12345,'Ar')

                if channel == 'tester':
                    extra = ['--target','Ar']
                elif channel == '1e':
                    extra = ['--electron-momentum','uniform','--electron-p-min','0','--electron-p-max',beam]
                elif channel in ('ep', 'en'):
                    # The pinned upstream generator draws p uniformly from 0.3 GeV/c to the beam
                    # energy and theta uniformly over the channel acceptance for both nucleons.
                    extra = ['--hadron-momentum','uniform',
                             '--hadron-p-min','0.3']
                else:
                    extra = []

                selection = [] if channel in ('1e','tester') else ['--hadron','proton' if channel=='ep' else 'neutron','--hadron-region','FD']

                run(current,'--channel','electron-tester' if channel == 'tester' else '1e' if channel == '1e' else 'eh','--beam-energy',beam,'--events',64,
                    '--seed',67890,'--vertex-seed',12345,'--A',1,'--Z',1,'--output',new_root,*selection,*extra)

                m=json.loads((new/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
                monitoring = new/'lundfiles/lund-gen-monitoring'/f"{m['config']['prefix']}_monitoring_plots.root"
                assert monitoring.is_file()

                # The 1e definitions retain exact archived names/ranges/content. Electron-hadron
                # definitions deliberately add FD/CD to hadron names and titles.
                if channel == '1e':
                    run(sys.argv[4], original/'histograms.root', monitoring)

                assert len(m['files']) == 1
                compare(new/m['files'][0]['path'],original/'legacy_1.txt', channel == 'tester')

        for target in ['liquid','4-foil','1-foil','1-foil-small','1-foil-large','Ca']:
            original,new_root=root/(target+'-old'),root/(target+'-new')
            new=uniform_output(new_root,'1e','2.07052')

            run(legacy,'1e',original,2.07052,64,1,67890,12345,target)
            run(current,'--channel','1e','--beam-energy',2.07052,'--target',target,'--A',1,'--Z',1,'--events',64,
                '--electron-momentum','uniform','--electron-p-min',0,'--electron-p-max',2.07052,
                '--output',new_root)

            m=json.loads((new/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())

            compare(new/m['files'][0]['path'],original/'legacy_1.txt')
    else:
        fixture=sys.argv[4]

        for beam,label,target,A,Z in [('2.07052','2070MeV','1-foil-small',12,6),('4.02962','4029MeV','1-foil-large',12,6),('5.98636','5986MeV','4-foil',12,6)]:
            gst=root/f'C12_GEM21_11a_00_000_{label}.root'

            run(fixture,gst,'parity')

            original,new_root=root/(label+'-old'),root/(label+'-new')

            run(legacy,gst,original,1,target,A,Z)
            run(current,'--input',gst,'--beam-energy',beam,'--target',target,'--A',A,'--Z',Z,'--events',10000,'--output',new_root)

            q2={'2.07052':'Q2_0_02','4.02962':'Q2_0_25','5.98636':'Q2_0_40'}[beam]
            new=new_root/f'rgm_fall2021_Ar__genie-unknown__unknown__{q2}__{label}_GEMC-unknown'
            m=json.loads((new/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
            assert not list((new/'lundfiles/lund-gen-monitoring').glob('*.root'))
            old=list((original/'lundfiles').glob('*.txt'))
            assert len(old)==1 and m['written_events']==10000
            compare(new/m['files'][0]['path'],old[0],drop_legacy_pi0=True,compare_mass_energy=True)

        # Demonstrate the corrected short-input behavior alongside the archived early-termination defect.
        gst=root/'C12_GEM21_11a_00_000_2070MeV_short.root'

        run(fixture,gst)

        original,new_root=root/'short-old',root/'short-new'

        run(legacy,gst,original,1,'1-foil-small',12,6)
        run(current,'--input',gst,'--beam-energy',2.07052,'--target','1-foil-small','--A',12,'--Z',6,'--events',10000,'--output',new_root)

        new=new_root/'rgm_fall2021_Ar__genie-unknown__unknown__Q2_0_02__2070MeV_GEMC-unknown'
        old_path=next((original/'lundfiles').glob('*.txt'))
        m=json.loads((new/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
        assert len(old_path.read_text().splitlines())==8 and m['scanned_events']==7 and m['written_events']==6
        assert [entry['events'] for entry in m['files']]==[6]
        print('Confirmed corrected cutoff: a short first file is completed through input exhaustion.')

print(mode+' legacy LUND parity passed')

# endregion
