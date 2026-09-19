#
# Created by Alon Sportes on 14/09/2026.
#

# CMakeLists.txt ------------------------------------------------------------
# Description:
#   Project configuration and build orchestration.
# Purpose:
#   Declare supported workflows, discover dependencies, embed provenance and connect libraries, applications and tests.
# Workflow:
#   CMake configure -> dependencies -> provenance header -> source targets -> optional tests -> install rules.
# Inputs:
#   CMake 3.20+, compatible ROOT, Threads, Git when available; BUILD_UNIFORM, BUILD_GENIE and BUILD_TESTING cache options.
# Outputs:
#   Build targets, generated/Version.h, compile_commands.json and installation rules.
# Notes:
#   ROOT must declare a C++ standard of at least 17. Configuration fails when required dependencies are unavailable.

# Project and workflow options -------------------------------------------------------

# region Project and workflow options
# Purpose: Select which compiled sample workflows are available.
cmake_minimum_required(VERSION 3.20)
project(CLAS12SampleGenerator VERSION 1.0.0 LANGUAGES CXX)
option(BUILD_UNIFORM "Build uniform sample generation" ON)
option(BUILD_GENIE "Build GENIE GST conversion" ON)
# endregion

# Compiler and dependency requirements -------------------------------------------------------

# region Compiler and dependency requirements
# Purpose: Match the selected ROOT language standard and propagate required imported targets.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ROOT_COMPONENTS Core RIO Physics)
if(BUILD_UNIFORM)
    list(APPEND ROOT_COMPONENTS Hist Gpad Graf)
endif()
if(BUILD_GENIE)
    list(APPEND ROOT_COMPONENTS Tree TreePlayer)
endif()
find_package(Threads REQUIRED)
find_package(ROOT REQUIRED COMPONENTS ${ROOT_COMPONENTS})
# Match the ABI/language configuration of the selected ROOT installation.
if(NOT ROOT_CXX_STANDARD OR ROOT_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "ROOT must publish ROOT_CXX_STANDARD >= 17; select a supported ROOT installation")
endif()
set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
# endregion

# Compiled provenance -------------------------------------------------------

# region Compiled provenance
# Purpose: Read the Git revision and target-header hash; configure a maintained header template. Protected inputs are never modified.
execute_process(COMMAND git describe --always --dirty
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE SAMPLE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT SAMPLE_REVISION)
    set(SAMPLE_REVISION "unknown")
endif()
# Reconfigure when the replaceable geometry source changes, including its manifest hash.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h")
file(SHA256 "${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h" SAMPLE_TARGETS_SHA256)
configure_file(src/lund-generation/core/support/Version.h.in generated/Version.h @ONLY)
# endregion

# Build graph and tests -------------------------------------------------------

# region Build graph and tests
# Purpose: Order subdirectories so libraries exist before applications and tests reference them.
include(CTest)
add_subdirectory(src)
# endregion

# Installation -------------------------------------------------------

# region Installation
# Purpose: Install shared configuration resources; each workflow source tree installs its own programs.
install(DIRECTORY config/ DESTINATION share/clas12-samples/config)
# endregion
