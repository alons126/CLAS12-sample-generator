#
# Created by Alon Sportes on 26/09/2026.
#

# src/workflows/CMakeLists.txt -------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Build entry point for user-facing workflow implementations.
#
# Purpose:
#   Keep each current and future workflow as a peer beneath one source boundary while allowing shared
#   workflow support to remain outside any individual workflow.
#
# Workflow:
#   src/CMakeLists.txt -> current workflow directories -> workflow-owned libraries and programs.
#
# Inputs:
#   Root build options and dependencies discovered before the source tree is added.
#
# Outputs:
#   Targets and installation rules owned by the currently implemented workflows.
#
# Failure behavior:
#   A child workflow configuration failure stops the root CMake configure step.

# Current workflows ------------------------------------------------------------------------------------------------------------------------------------------------------

# region Current workflows
add_subdirectory(lund-creation)
add_subdirectory(slurm-submission)
# endregion
