# apps/CMakeLists.txt ------------------------------------------------------------
# Description:
#   Command-line executable targets.
# Purpose:
#   Connect small CLI entry points to their implementation libraries and install enabled applications.
# Workflow:
#   src/ creates libraries -> this directory links executables -> CMake install places them under bin/.
# Inputs:
#   BUILD_UNIFORM and BUILD_GENIE, plus the libraries defined in src/.
# Outputs:
#   clas12-uniform and/or clas12-genie-to-lund in the build apps/ directory.
# Notes:
#   Defining or building a target does not generate samples or submit detector jobs.

# Uniform CLI -------------------------------------------------------
# region Uniform CLI
# Purpose: Link the uniform entry point to the implementation library and install its executable.
if(BUILD_UNIFORM)
    add_executable(clas12-uniform uniform_main.cpp)
    target_link_libraries(clas12-uniform PRIVATE UniformGeneration)
    install(TARGETS clas12-uniform RUNTIME DESTINATION bin)
endif()
# endregion

# GENIE conversion CLI -------------------------------------------------------
# region GENIE conversion CLI
# Purpose: Link the GST conversion entry point and install its executable.
if(BUILD_GENIE)
    add_executable(clas12-genie-to-lund genie_to_lund_main.cpp)
    target_link_libraries(clas12-genie-to-lund PRIVATE GenieConversion)
    install(TARGETS clas12-genie-to-lund RUNTIME DESTINATION bin)
endif()
# endregion
