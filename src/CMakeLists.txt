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
#   Source-specific directories match their executable names. Physical event-generator adapters are nested below
#   clas12-generator-to-lund; LundCore remains one target because its shared layers form one small pipeline.

# Shared LUND pipeline -------------------------------------------------------

# region Shared LUND pipeline
# Purpose: Join configuration, geometry, LUND records/serialization, and support definitions without hiding their source boundaries.
add_library(
    LundCore STATIC
    config/RunConfig.cpp
    config/RgmTarget.cpp
    geometry/TargetGeometry.cpp
    lund/Particle.cpp
    lund/LundWriter.cpp)
target_include_directories(LundCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(LundCore PUBLIC ROOT::Core ROOT::Physics PRIVATE Threads::Threads ROOT::RIO)
# endregion

# Uniform generation -------------------------------------------------------

# region Uniform generation
# Purpose: Build the implementation behind the clas12-uniform executable as UniformGeneration.
if(BUILD_UNIFORM)
    add_library(UniformGeneration STATIC clas12-uniform/UniformGenerator.cpp clas12-uniform/UniformMonitoring.cpp)
    target_link_libraries(UniformGeneration PUBLIC LundCore PRIVATE ROOT::RIO ROOT::Hist ROOT::Gpad ROOT::Graf)
endif()
# endregion

# Physical-event conversion -------------------------------------------------------

# region Physical-event conversion
# Purpose: Build the clas12-generator-to-lund dispatcher and keep GST dependencies on its nested GENIE adapter.
if(BUILD_GENIE)
    add_library(GenieConversion STATIC clas12-generator-to-lund/genie/GenieConverter.cpp)
    target_link_libraries(GenieConversion PUBLIC LundCore PRIVATE ROOT::Tree ROOT::TreePlayer ROOT::RIO)
    add_library(PhysicalConversion STATIC clas12-generator-to-lund/PhysicalConverter.cpp)
    target_link_libraries(PhysicalConversion PUBLIC LundCore PRIVATE GenieConversion)
endif()
# endregion
