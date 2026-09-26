#
# Created by Alon Sportes on 19/09/2026.
#

# Ifarm submission -------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#     Install the external worker used by submitted array tasks.
#
# Purpose:
#     Publish the protected detector worker without duplicating its commands.
#
# Workflow:
#     run.csh -> setup_and_submit.csh -> submit.py -> sbatch array -> external payload.
#
# Inputs:
#     The protected external worker script.
#
# Outputs:
#     The worker is installed under bin.
#
# Failure:
#     Installation fails when CMake cannot copy the worker to the selected prefix.

# region Installation
install(PROGRAMS external/submit_GEMC_sample.sh DESTINATION bin)
# endregion

# region Optional local development integration
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/resolution.py"
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/submission_parity.py")
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    if(BUILD_UNIFORM)
        add_test(NAME submission-input-resolution COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/resolution.py ${PROJECT_SOURCE_DIR} $<TARGET_FILE:uniform-lund-generator>)
    else()
        add_test(NAME submission-input-resolution COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/resolution.py ${PROJECT_SOURCE_DIR})
    endif()
    find_program(TCSH_EXECUTABLE NAMES tcsh csh)
    if(TCSH_EXECUTABLE)
        add_test(NAME submission-legacy-parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/submission_parity.py ${PROJECT_SOURCE_DIR} ${TCSH_EXECUTABLE})
    endif()
endif()
# endregion
