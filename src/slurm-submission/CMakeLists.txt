#
# Created by Alon Sportes on 19/09/2026.
#

# Ifarm submission ------------------------------------------------------------
# Purpose: register tests for the sourced setup and protected detector payload.
# Workflow: run.csh -> setup_and_submit.csh -> sbatch array -> external payload.
# Inputs: Python and tcsh for isolated tests; no detector execution or real submission.
# Outputs: payload installed under bin; setup runs from the server checkout.
# Failure: a failed parity or safety assertion fails CTest.

# region Installation
install(PROGRAMS external/submit_GEMC_sample.sh DESTINATION bin)
# endregion

# region Tests
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    find_program(TCSH_EXECUTABLE NAMES tcsh csh)
    if(TCSH_EXECUTABLE)
        add_test(NAME submission-legacy-parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/submission_parity.py ${PROJECT_SOURCE_DIR} ${TCSH_EXECUTABLE})
    endif()
endif()
# endregion
