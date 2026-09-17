#
# Created by Alon Sportes on 14/09/2026.
#

# src/CMakeLists.txt ------------------------------------------------------------
# Description:
#   Reusable C++ libraries.
# Purpose:
#   Compile shared infrastructure once and link the selected sample-generation implementations against it.
# Workflow:
#   The root discovers ROOT/Threads first; this directory creates libraries consumed by apps/ and tests/.
# Inputs:
#   Root workflow options, imported ROOT/Threads targets and the generated provenance include directory.
# Outputs:
#   SampleCommon, optional UniformGeneration and optional GenieConversion static libraries.
# Notes:
#   PUBLIC dependencies propagate to consumers; PRIVATE dependencies belong to library implementation.

# Shared infrastructure -------------------------------------------------------

# region Shared infrastructure
# Purpose: Own configuration, target adapter, particle masses, output and diagnostics in one static library.
add_library(SampleCommon STATIC common/RunConfig.cpp common/RgmTarget.cpp common/TargetGeometry.cpp common/Particle.cpp common/LundWriter.cpp common/Monitoring.cpp common/LegacyMonitoring.cpp)
target_include_directories(SampleCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(SampleCommon PUBLIC ROOT::Core ROOT::Physics PRIVATE Threads::Threads ROOT::RIO ROOT::Hist ROOT::Gpad ROOT::Graf)
# endregion

# Uniform generation -------------------------------------------------------

# region Uniform generation
# Purpose: Expose the acceptance-sample implementation through UniformGeneration.
if(BUILD_UNIFORM)
    add_library(UniformGeneration STATIC uniform/UniformGenerator.cpp)
    target_link_libraries(UniformGeneration PUBLIC SampleCommon)
endif()
# endregion

# Physical-event conversion -------------------------------------------------------

# region Physical-event conversion
# Purpose: Add GST tree-reading dependencies only to the GENIE conversion implementation.
if(BUILD_GENIE)
    add_library(GenieConversion STATIC genie/GenieConverter.cpp)
    target_link_libraries(GenieConversion PUBLIC SampleCommon PRIVATE ROOT::Tree ROOT::TreePlayer ROOT::RIO)
    add_library(PhysicalConversion STATIC physical/PhysicalConverter.cpp)
    target_link_libraries(PhysicalConversion PUBLIC SampleCommon PRIVATE GenieConversion)
endif()
# endregion
