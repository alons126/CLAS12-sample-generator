#
# Created by Alon Sportes on 19/09/2026.
#

# src/slurm-submission/CMakeLists.txt --------------------------------------------------------------------------------------------------------
# Description:
#   Installation and validation rules for ifarm Slurm submission.
# Purpose:
#   Install the manifest runner, array submitter, and protected GEMC payload; register submission tests.
# Workflow:
#   Completed LUND manifest -> submit.py -> Slurm task -> run.py -> external payload.
# Inputs:
#   Python 3.9+, an enabled uniform executable for integration fixtures, and protected payload source.
# Outputs:
#   clas12-simulate, clas12-submit, submit_GEMC_sample.sh, and submission CTest cases.
# Failure behavior:
#   Missing Python or failed test registration stops configuration.

# Installation --------------------------------------------------------------------------------------------------------------------------------

# region Installation
install(PROGRAMS external/submit_GEMC_sample.sh DESTINATION bin)
install(PROGRAMS run.py DESTINATION bin RENAME clas12-simulate)
install(PROGRAMS submit.py DESTINATION bin RENAME clas12-submit)
# endregion

# Submission tests ---------------------------------------------------------------------------------------------------------------------------

# region Submission tests
if(BUILD_TESTING)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    add_test(NAME submission-legacy-parity COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/submission_parity.py ${PROJECT_SOURCE_DIR})
    if(BUILD_UNIFORM)
        add_test(NAME simulation-integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/simulation.py $<TARGET_FILE:clas12-uniform> ${PROJECT_SOURCE_DIR})
    endif()
endif()
# endregion
