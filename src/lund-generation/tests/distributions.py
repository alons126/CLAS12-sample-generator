#
# Created by Alon Sportes on 14/09/2026.
#

"""Compare generated uniform samples with their expected distributions.

Purpose:
    Check the electron and proton mixture parts and the flat neutron distribution.

Workflow:
    CTest supplies the executable -> temporary runs create samples -> numerical checks report failures.

Notes:
    Tests use temporary files and do not change external or archived sources.
"""

import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

# ks --------------------------------------------------------------------
# region ks
def ks(values, cdf):
    """Check sampled values against an expected cumulative distribution.

    Algorithm:
        Sort the values and measure the largest distance from the expected cumulative distribution.

    Args:
        values: Sampled values for one distribution.
        cdf: Function that returns the expected fraction at a value.

    Returns:
        Nothing. A distance of 0.025 or more raises an assertion.
    """

    values=sorted(values)
    n=len(values)
    distance=max(max(abs(cdf(x)-i/n), abs((i+1)/n-cdf(x))) for i,x in enumerate(values))
    assert distance < 0.025, f'CDF distance {distance}'
# endregion

# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-distributions-') as temp:
    # The default 1e sample alternates flat momentum and flat inverse momentum from 0.7 to the beam value.
    output_root=Path(temp)/'1e'
    out=output_root/'Uniform_sample_1e_5986MeV'

    subprocess.run([sys.argv[1],'--channel','1e','--events','20000','--output',str(output_root)],check=True,capture_output=True)

    m=json.loads((out/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
    lines=(out/m['files'][0]['path']).read_text().splitlines()
    momenta=[]

    for i in range(0,len(lines),2):
        p=list(map(float,lines[i+1].split()))
        x,y,z=p[6:9]; momenta.append(math.sqrt(x*x+y*y+z*z))

    assert m['config']['electron-momentum']=='mixed' and m['config']['electron-p-min']=='0.7'
    beam=5.98636

    ks(momenta[::2],lambda p:(p-0.7)/(beam-0.7))
    ks([1/p for p in momenta[1::2]],lambda q:(q-1/beam)/(1/0.7-1/beam))

    for channel in ['enFD','epFD']:
        output_root=Path(temp)/channel
        out=output_root/f'Uniform_sample_{channel}_5986MeV'
        p_min='0' if channel=='enFD' else '0.3'
        hadron='neutron' if channel=='enFD' else 'proton'

        subprocess.run([sys.argv[1],'--channel','eh','--hadron',hadron,'--hadron-region','FD','--hadron-momentum','sampled','--hadron-p-min',p_min,
                        '--events','20000','--output',str(output_root)],check=True,capture_output=True)

        m=json.loads((out/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
        lines=(out/m['files'][0]['path']).read_text().splitlines()
        momenta,cosines,phis=[],[],[]

        for i in range(0,len(lines),3):
            p=list(map(float,lines[i+2].split()))
            x,y,z=p[6:9]; mag=math.sqrt(x*x+y*y+z*z)

            momenta.append(mag);cosines.append(z/mag);phis.append(math.atan2(y,x))

        max_theta=35 if channel=='enFD' else 45
        lo,hi=math.cos(math.radians(max_theta)),math.cos(math.radians(5))
        stable_cosines=[c for p,c in zip(momenta,cosines) if p > 0.01]
        stable_phis=[phi for p,phi in zip(momenta,phis) if p > 0.01]
        assert min(stable_cosines)>=lo-2e-4 and max(stable_cosines)<=hi+2e-4
        ks(stable_phis,lambda x:(x+math.pi)/(2*math.pi))

        if channel=='enFD':
            ks([math.degrees(math.acos(c)) for c in stable_cosines],lambda t:(t-5)/30)
            ks(momenta,lambda p:p/beam)
        else:
            assert m['config']['hadron-momentum']=='mixed'
            ks([math.degrees(math.acos(c)) for c in stable_cosines],lambda t:(t-5)/40)
            ks(momenta[::2],lambda p:(p-0.3)/(beam-0.3))
            ks([1/p for p in momenta[1::2]],lambda q:(q-1/beam)/(1/0.3-1/beam))
            ks(momenta,lambda p:0.5*(p-0.3)/(beam-0.3)+0.5*(1/0.3-1/p)/(1/0.3-1/beam))

print('Electron/proton mixtures and uniform neutron momentum passed')

# endregion
