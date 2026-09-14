"""Change only the drop-in external header for an independent adapter build."""
from pathlib import Path
import sys
source, output = map(Path, sys.argv[1:])
s = source.read_text()
# These assertions make a reference-header update fail visibly until its test
# modifications are reviewed. No production geometry code is rewritten here.
assert 'double global_z = -3;' in s and 'double beamspot_x = 0.;' in s
s = s.replace('double global_z = -3;', 'double global_z = -13;')
s = s.replace('double beamspot_x = 0.;', 'double beamspot_x = 0.75;')
s = s.replace('{"Ca",', '{"replacement-only", {-12.0}},\n  {"Ca",')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(s)
