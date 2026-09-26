#
# Created by Alon Sportes on 19/09/2026.
#

# src/launcher/CMakeLists.txt --------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Build boundary for the shared local and ifarm launcher.
# 
# Purpose:
#   Keep launcher-specific optional development integration separate from production targets.
# 
# Workflow:
#   run.csh -> launcher/workflow.py -> selected LUND source; submission is sourced directly.
# 
# Inputs:
#   The configured source and build trees.
# 
# Outputs:
#   No installed target; runtime launcher files are used directly from the checkout.
# 
# Failure behavior:
#   Optional local development integration is skipped when its files or tools are unavailable.

# Optional local development integration --------------------------------------------------------------------------------------------------------------------------------

# region Optional local development integration
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher.py")
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    find_program(TCSH_EXECUTABLE NAMES tcsh csh)
    if(TCSH_EXECUTABLE AND BUILD_UNIFORM AND BUILD_GENIE)
        add_test(NAME ssh-launcher COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher.py
            ${PROJECT_SOURCE_DIR} ${TCSH_EXECUTABLE} ${PROJECT_BINARY_DIR})
    endif()
endif()
# endregion
