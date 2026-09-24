#
# Created by Alon Sportes on 14/09/2026.
#

# src/lund-generation/CMakeLists.txt -------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Responsibility-grouped C++ libraries.
#
# Purpose:
#   Compile the shared LUND pipeline once and link source-specific generation or conversion code against it.
#
# Workflow:
#   The root discovers ROOT/Threads first; this directory creates libraries consumed by apps/ and tests/.
#
# Inputs:
#   Root workflow options, imported ROOT/Threads targets and the generated provenance include directory.
#
# Outputs:
#   LundCore, optional UniformGeneration, and optional GenieGstConversion/PhysicalConversion static libraries.
#
# Notes:
#   Converter directories describe their implementation responsibility rather than executable names. Physical
#   event-generator format adapters are nested below event-generator-to-lund-converter; LundCore remains one target
#   because its shared layers form one small pipeline.

# Shared LUND pipeline ---------------------------------------------------------------------------------------------------------------------------------------------------

# region Shared LUND pipeline
# Purpose:
#     Join the core configuration, geometry, LUND records/serialization, and support layers without hiding their source boundaries.
add_library(
    LundCore STATIC
    core/config/RunConfig.cpp
    core/config/RgmTarget.cpp
    core/geometry/TargetGeometry.cpp
    core/lund/Particle.cpp
    core/lund/LundWriter.cpp)
target_include_directories(LundCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(LundCore PUBLIC ROOT::Core ROOT::Physics PRIVATE Threads::Threads ROOT::RIO)
# endregion

# Uniform generation -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Uniform generation
# Purpose:
#     Build the implementation behind the uniform-lund-generator executable as UniformGeneration.
if(BUILD_UNIFORM)
    add_library(UniformGeneration STATIC uniform-lund-generator/UniformGenerator.cpp uniform-lund-generator/UniformMonitoring.cpp)
    target_link_libraries(UniformGeneration PUBLIC LundCore PRIVATE ROOT::RIO ROOT::Hist ROOT::Gpad ROOT::Graf)
endif()
# endregion

# Physical-event conversion ----------------------------------------------------------------------------------------------------------------------------------------------

# region Physical-event conversion
# Purpose:
#     Build the event-generator-to-lund-converter dispatcher and keep ROOT GST dependencies on its nested GENIE GST adapter.
if(BUILD_GENIE)
    add_library(GenieGstConversion STATIC event-generator-to-lund-converter/genie-gst/GenieConverter.cpp)
    target_link_libraries(GenieGstConversion PUBLIC LundCore PRIVATE ROOT::Tree ROOT::TreePlayer ROOT::RIO)
    add_library(PhysicalConversion STATIC event-generator-to-lund-converter/PhysicalConverter.cpp)
    target_link_libraries(PhysicalConversion PUBLIC LundCore PRIVATE GenieGstConversion)
endif()
# endregion

# Applications and tests -------------------------------------------------------------------------------------------------------------------------------------------------

# region Applications and tests
# Purpose:
#     Keep LUND executable entry points and validation beside the implementation they exercise.
add_subdirectory(apps)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
# endregion
