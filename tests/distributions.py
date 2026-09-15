"""Check sampled nucleon distributions against analytic CDFs.

Purpose:
    Test angular and momentum prescriptions, including both proton-mixture components.

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


# ks --------------------------------------------------------------------
# region ks
def ks(values, cdf):
    """Check empirical draws against an analytical CDF.

    Algorithm:
        Sort values and compare both sides of each empirical CDF jump against the 0.025 bound.

    Args:
        values: Sampled values for one distribution.
        cdf: Expected cumulative distribution function.

    Returns:
        None; excessive CDF distance raises an assertion.
    """
    values=sorted(values)
    n=len(values)
    distance=max(max(abs(cdf(x)-i/n), abs((i+1)/n-cdf(x))) for i,x in enumerate(values))
    assert distance < 0.025, f'CDF distance {distance}'
# endregion


# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-distributions-') as temp:
    for channel in ['en','ep']:
        output_root=Path(temp)/channel
        out=output_root/f'Uniform_sample_{channel}_5986MeV'
        subprocess.run([sys.argv[1],'--channel',channel,'--nucleon-momentum','sampled','--nucleon-p-min','0.3','--nucleon-p-max','3',
                        '--events','20000','--lund-format','precise','--output',str(output_root)],check=True,capture_output=True)
        m=json.loads((out/'manifest.json').read_text())
        lines=(out/m['files'][0]['path']).read_text().splitlines()
        momenta,cosines,phis=[],[],[]
        for i in range(0,len(lines),3):
            p=list(map(float,lines[i+2].split()))
            x,y,z=p[6:9]; mag=math.sqrt(x*x+y*y+z*z)
            momenta.append(mag);cosines.append(z/mag);phis.append(math.atan2(y,x))
        max_theta=35 if channel=='en' else 45
        lo,hi=math.cos(math.radians(max_theta)),math.cos(math.radians(5))
        assert min(cosines)>=lo-1e-9 and max(cosines)<=hi+1e-9
        ks(phis,lambda x:(x+math.pi)/(2*math.pi))
        if channel=='en':
            assert m['config']['nucleon-angle']=='isotropic'
            ks(cosines,lambda x:(x-lo)/(hi-lo))
            ks(momenta,lambda p:(p-0.3)/2.7)
        else:
            assert m['config']['nucleon-momentum']=='mixed' and m['config']['nucleon-angle']=='theta'
            ks([math.degrees(math.acos(c)) for c in cosines],lambda t:(t-5)/40)
            ks(momenta[::2],lambda p:(p-0.3)/2.7)
            ks([1/p for p in momenta[1::2]],lambda q:(q-1/3)/(1/0.3-1/3))
            ks(momenta,lambda p:0.5*(p-0.3)/2.7+0.5*(1/0.3-1/p)/(1/0.3-1/3))
print('Neutron isotropy and proton 50/50 momentum mixture passed')

# endregion
