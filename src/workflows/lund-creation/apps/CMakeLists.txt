#
# Created by Alon Sportes on 14/09/2026.
#

# src/workflows/lund-creation/apps/CMakeLists.txt ------------------------------------------------------------------------------------------------------------------------
# Description:
#   Command-line executable targets.
# 
# Purpose:
#   Link each small command-line program to its library and install enabled programs.
# 
# Workflow:
#   src/ creates libraries -> this directory links executables -> CMake install places them under bin/.
# 
# Inputs:
#   BUILD_UNIFORM and BUILD_GENIE, plus the libraries defined in src/.
# 
# Outputs:
#   uniform-lund-creator and/or event-generator-to-lund-converter in the build apps/ directory.
# 
# Notes:
#   Defining or building a target does not generate samples or submit detector jobs.

# Uniform CLI ------------------------------------------------------------------------------------------------------------------------------------------------------------

# region Uniform CLI
# Purpose:
#     Link the uniform entry point to the implementation library and install its executable.
if(BUILD_UNIFORM)
    add_executable(uniform-lund-creator uniform_lund_creator_main.cpp)
    target_link_libraries(uniform-lund-creator PRIVATE UniformGeneration)
    set_target_properties(uniform-lund-creator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/apps)
    install(TARGETS uniform-lund-creator RUNTIME DESTINATION bin)
endif()
# endregion

# Physical conversion CLI ------------------------------------------------------------------------------------------------------------------------------------------------

# region Physical conversion CLI
# Purpose:
#     Link physical conversion and install its format-independent command-line program.
if(BUILD_GENIE)
    add_executable(event-generator-to-lund-converter event_generator_to_lund_converter_main.cpp)
    target_link_libraries(event-generator-to-lund-converter PRIVATE PhysicalConversion)
    set_target_properties(event-generator-to-lund-converter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/apps)
    install(TARGETS event-generator-to-lund-converter RUNTIME DESTINATION bin)
endif()
# endregion
