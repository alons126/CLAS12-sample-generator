#
# Created by Alon Sportes on 19/09/2026.
#

# src/CMakeLists.txt -----------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Entry point for the source subdirectories.
# 
# Purpose:
#   Separate user-facing workflows from the launcher that selects them.
# 
# Workflow:
#   Root configuration -> workflow targets -> launcher integration.
# 
# Inputs:
#   Root workflow options and imported dependencies.
# 
# Outputs:
#   Compiled LUND applications and installed submission programs.
# 
# Failure behavior:
#   A child configuration failure stops the root CMake configure step.

# Source boundaries ------------------------------------------------------------------------------------------------------------------------------------------------------

# region Source boundaries
add_subdirectory(workflows)
add_subdirectory(launcher)
# endregion
