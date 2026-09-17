#
# Created by Alon Sportes on 14/09/2026.
#

# src/CMakeLists.txt ------------------------------------------------------------
# Description:
#   Responsibility-grouped C++ libraries.
# Purpose:
#   Compile the shared LUND pipeline once and link source-specific generation or conversion code against it.
# Workflow:
#   The root discovers ROOT/Threads first; this directory creates libraries consumed by apps/ and tests/.
# Inputs:
#   Root workflow options, imported ROOT/Threads targets and the generated provenance include directory.
# Outputs:
#   LundCore, optional UniformGeneration, and optional GenieConversion/PhysicalConversion static libraries.
# Notes:
#   The directories describe source ownership; LundCore remains one target because these layers form one small pipeline.

# Shared LUND pipeline -------------------------------------------------------

# region Shared LUND pipeline
# Purpose: Join configuration, geometry, LUND records/serialization, monitoring, and support definitions without hiding their source boundaries.
add_library(
    LundCore STATIC
    config/RunConfig.cpp
    config/RgmTarget.cpp
    geometry/TargetGeometry.cpp
    lund/Particle.cpp
    lund/LundWriter.cpp
    monitoring/Monitoring.cpp
    monitoring/LegacyMonitoring.cpp)
target_include_directories(LundCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(LundCore PUBLIC ROOT::Core ROOT::Physics PRIVATE Threads::Threads ROOT::RIO ROOT::Hist ROOT::Gpad ROOT::Graf)
# endregion

# Uniform generation -------------------------------------------------------

# region Uniform generation
# Purpose: Expose the acceptance-sample implementation through UniformGeneration.
if(BUILD_UNIFORM)
    add_library(UniformGeneration STATIC uniform/UniformGenerator.cpp)
    target_link_libraries(UniformGeneration PUBLIC LundCore)
endif()
# endregion

# Physical-event conversion -------------------------------------------------------

# region Physical-event conversion
# Purpose: Add GST tree-reading dependencies only to the GENIE conversion implementation.
if(BUILD_GENIE)
    add_library(GenieConversion STATIC genie/GenieConverter.cpp)
    target_link_libraries(GenieConversion PUBLIC LundCore PRIVATE ROOT::Tree ROOT::TreePlayer ROOT::RIO)
    add_library(PhysicalConversion STATIC physical/PhysicalConverter.cpp)
    target_link_libraries(PhysicalConversion PUBLIC LundCore PRIVATE GenieConversion)
endif()
# endregion
