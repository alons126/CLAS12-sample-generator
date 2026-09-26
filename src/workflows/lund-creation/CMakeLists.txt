#
# Created by Alon Sportes on 14/09/2026.
#

# src/workflows/lund-creation/CMakeLists.txt -----------------------------------------------------------------------------------------------------------------------------
# Description:
#   C++ libraries grouped by their job.
#
# Purpose:
#   Compile the shared LUND pipeline once and link source-specific generation or conversion code against it.
#
# Workflow:
#   The root discovers ROOT/Threads first; this directory creates libraries consumed by the applications.
#
# Inputs:
#   Root workflow options, imported ROOT/Threads targets and the generated provenance include directory.
#
# Outputs:
#   Shared and source-specific libraries plus the enabled LUND command-line applications.
#
# Notes:
#   Format-specific converters are kept below event-generator-to-lund-converter. LundCore remains one
#   small shared library used by both LUND sources. Application sources stay in apps/ while their two
#   target definitions remain here with the libraries they consume.

# Shared LUND pipeline ---------------------------------------------------------------------------------------------------------------------------------------------------

# region Shared LUND pipeline
# Purpose:
#     Build the shared configuration, geometry, and LUND writing code as one library.
add_library(
    LundCore STATIC
    core/config/RunConfig.cpp
    core/config/RgmTarget.cpp
    core/geometry/TargetGeometry.cpp
    core/lund/Particle.cpp
    core/lund/LundWriter.cpp)
target_include_directories(LundCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/workflows PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(LundCore PUBLIC ROOT::Core ROOT::Physics PRIVATE Threads::Threads ROOT::RIO)
# endregion

# Uniform generation -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Uniform generation
# Purpose:
#     Build the implementation behind the uniform-lund-creator executable as UniformGeneration.
if(BUILD_UNIFORM)
    add_library(UniformGeneration STATIC uniform-lund-creator/UniformGenerator.cpp uniform-lund-creator/UniformMonitoring.cpp)
    target_link_libraries(UniformGeneration PUBLIC LundCore PRIVATE ROOT::RIO ROOT::Hist ROOT::Gpad ROOT::Graf)
endif()
# endregion

# Physical-event conversion ----------------------------------------------------------------------------------------------------------------------------------------------

# region Physical-event conversion
# Purpose:
#     Build physical conversion and keep ROOT tree libraries inside the GENIE GST adapter.
if(BUILD_GENIE)
    add_library(GenieGstConversion STATIC event-generator-to-lund-converter/genie-gst/GenieConverterGST.cpp)
    target_link_libraries(GenieGstConversion PUBLIC LundCore PRIVATE ROOT::Tree ROOT::TreePlayer ROOT::RIO)
    add_library(PhysicalConversion STATIC event-generator-to-lund-converter/PhysicalConverter.cpp)
    target_link_libraries(PhysicalConversion PUBLIC LundCore PRIVATE GenieGstConversion)
endif()
# endregion

# Uniform CLI ------------------------------------------------------------------------------------------------------------------------------------------------------------

# region Uniform CLI
# Purpose:
#     Link the uniform entry point to its implementation library and install the executable.
if(BUILD_UNIFORM)
    add_executable(uniform-lund-creator apps/uniform_lund_creator_main.cpp)
    target_link_libraries(uniform-lund-creator PRIVATE UniformGeneration)
    set_target_properties(uniform-lund-creator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/apps)
    install(TARGETS uniform-lund-creator RUNTIME DESTINATION bin)
endif()
# endregion

# Physical conversion CLI ------------------------------------------------------------------------------------------------------------------------------------------------

# region Physical conversion CLI
# Purpose:
#     Link the format-independent physical entry point and install the executable.
if(BUILD_GENIE)
    add_executable(event-generator-to-lund-converter apps/event_generator_to_lund_converter_main.cpp)
    target_link_libraries(event-generator-to-lund-converter PRIVATE PhysicalConversion)
    set_target_properties(event-generator-to-lund-converter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/apps)
    install(TARGETS event-generator-to-lund-converter RUNTIME DESTINATION bin)
endif()
# endregion

# Local validation -------------------------------------------------------------------------------------------------------------------------------------------------------

# region Local validation
# Purpose:
#     Add workstation-only tests when their intentionally untracked CMake entry point is present.
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
# endregion
