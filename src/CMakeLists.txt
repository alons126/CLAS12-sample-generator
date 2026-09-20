#
# Created by Alon Sportes on 19/09/2026.
#

# src/CMakeLists.txt -----------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Workflow-oriented source-tree entry point.
# Purpose:
#   Keep LUND generation, Slurm submission, and their shared launcher visibly separated below src/.
# Workflow:
#   Root configuration -> LUND targets/tests -> submission coordinators/tests -> shared launcher test.
# Inputs:
#   Root workflow options, imported dependencies, and BUILD_TESTING.
# Outputs:
#   Compiled LUND applications, installed submission programs, and registered workflow tests.
# Failure behavior:
#   A child configuration failure stops the root CMake configure step.

# Workflow source trees --------------------------------------------------------------------------------------------------------------------------------------------------

# region Workflow source trees
add_subdirectory(lund-generation)
add_subdirectory(slurm-submission)
add_subdirectory(launcher)
# endregion
