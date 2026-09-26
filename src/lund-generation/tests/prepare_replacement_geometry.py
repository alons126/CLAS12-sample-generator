#
# Created by Alon Sportes on 14/09/2026.
#

"""Build a separate target-header copy for the geometry replacement test.

Purpose:
    Read the external target header and change only the copy in the test build directory.

Workflow:
    CTest supplies both paths -> this script checks known source text -> it writes the changed copy.

Notes:
    The external source stays unchanged.
"""

from pathlib import Path
import sys
# Fixture generation -----------------------------------------------------------
# region Fixture generation
source, output = map(Path, sys.argv[1:])
s = source.read_text()
# Stop when the expected source text changes so the test update must be reviewed.
assert 'double global_z = -3;' in s and 'double beamspot_x = 0.;' in s
s = s.replace('double global_z = -3;', 'double global_z = -13;')
s = s.replace('double beamspot_x = 0.;', 'double beamspot_x = 0.75;')
s = s.replace('{"Ca",', '{"replacement-only", {-12.0}},\n  {"Ca",')

output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(s)

# endregion
