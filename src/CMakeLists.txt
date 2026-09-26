#
# Created by Alon Sportes on 19/09/2026.
#

# src/CMakeLists.txt -----------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Entry point for the source subdirectories.
# 
# Purpose:
#   Keep LUND generation, Slurm submission, and the shared launcher in separate directories.
# 
# Workflow:
#   Root configuration -> LUND targets -> submission coordinators -> shared launcher.
# 
# Inputs:
#   Root workflow options and imported dependencies.
# 
# Outputs:
#   Compiled LUND applications and installed submission programs.
# 
# Failure behavior:
#   A child configuration failure stops the root CMake configure step.

# Workflow source trees --------------------------------------------------------------------------------------------------------------------------------------------------

# region Workflow source trees
add_subdirectory(lund-generation)
add_subdirectory(slurm-submission)
add_subdirectory(launcher)
# endregion
