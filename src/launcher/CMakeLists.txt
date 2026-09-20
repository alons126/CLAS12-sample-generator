#
# Created by Alon Sportes on 19/09/2026.
#

# src/launcher/CMakeLists.txt --------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Test registration for the shared local/ifarm workflow dispatcher.
# 
# Purpose:
#   Verify sourced-shell behavior, build dispatch, argument preservation, and disposable-clone safeguards.
# 
# Workflow:
#   run.csh -> launcher/workflow.py -> selected LUND source; submission is sourced directly.
# 
# Inputs:
#   Python 3.9+, tcsh/csh when available, both enabled LUND applications, and the configured build tree.
# 
# Outputs:
#   The optional ssh-launcher CTest case.
# 
# Failure behavior:
#   The launcher test is omitted when its shell or compiled applications are unavailable.

# Launcher integration test ----------------------------------------------------------------------------------------------------------------------------------------------

# region Launcher integration test
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    find_program(TCSH_EXECUTABLE NAMES tcsh csh)
    if(TCSH_EXECUTABLE AND BUILD_UNIFORM AND BUILD_GENIE)
        add_test(NAME ssh-launcher COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher.py
            ${PROJECT_SOURCE_DIR} ${TCSH_EXECUTABLE} ${PROJECT_BINARY_DIR})
    endif()
endif()
# endregion
