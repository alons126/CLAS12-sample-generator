#
# Created by Alon Sportes on 14/09/2026.
#

# CMakeLists.txt ---------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Top-level project build settings.
# 
# Purpose:
#   Select workflows, find dependencies, record build details, and add the production source targets.
# 
# Workflow:
#   CMake configure -> dependencies -> provenance header -> source targets -> install rules.
# 
# Inputs:
#   CMake 3.20+, compatible ROOT, Threads, Git when available; BUILD_UNIFORM and BUILD_GENIE cache options.
# 
# Outputs:
#   Build targets, generated/Version.h, compile_commands.json and installation rules.
# 
# Notes:
#   ROOT must declare a C++ standard of at least 17. Configuration fails when required dependencies are unavailable.

# Project and workflow options -------------------------------------------------------------------------------------------------------------------------------------------

# region Project and workflow options
# Purpose: Select which compiled sample workflows are available.
cmake_minimum_required(VERSION 3.20)
project(CLAS12SampleGenerator VERSION 1.0.0 LANGUAGES CXX)
option(BUILD_UNIFORM "Build uniform sample generation" ON)
option(BUILD_GENIE "Build GENIE GST conversion" ON)
# endregion

# Compiler and dependency requirements -----------------------------------------------------------------------------------------------------------------------------------

# region Compiler and dependency requirements
# Purpose: Use the C++ standard required by ROOT and load the needed ROOT libraries.
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
# Use the same C++ standard as the selected ROOT installation.
if(NOT ROOT_CXX_STANDARD OR ROOT_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "ROOT must publish ROOT_CXX_STANDARD >= 17; select a supported ROOT installation")
endif()
set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
# endregion

# Compiled provenance ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Compiled provenance
# Purpose: Read Git/build details and the target-header hash, then write the generated version header.
# read_git_value:
#   Read one Git value from the source checkout and escape it for a generated C++ string literal.
#
# Inputs:
#   The destination CMake variable followed by the Git subcommand and its arguments.
#
# Outputs:
#   One parent-scope string, using `Not available` when Git cannot provide the requested value.
#
# Failure:
#   Git errors remain nonfatal so source archives without repository metadata can still build.
function(read_git_value output)
    execute_process(COMMAND git ${ARGN}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE value
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT result EQUAL 0 OR NOT value)
        set(value "Not available")
    endif()
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    string(REPLACE "\n" " | " value "${value}")
    string(REPLACE "\r" "" value "${value}")
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

execute_process(COMMAND git describe --always --dirty
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE SAMPLE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT SAMPLE_REVISION)
    set(SAMPLE_REVISION "unknown")
endif()
read_git_value(SAMPLE_GIT_REPOSITORY remote get-url origin)
read_git_value(SAMPLE_GIT_BRANCH branch --show-current)
read_git_value(SAMPLE_GIT_COMMIT_MESSAGE log -1 --format=%s)
read_git_value(SAMPLE_GIT_COMMIT_HASH rev-parse HEAD)
read_git_value(SAMPLE_GIT_COMMIT_DATETIME log -1 --format=%cI)
read_git_value(SAMPLE_GIT_COMMIT_AUTHOR log -1 --format=%an)
read_git_value(SAMPLE_GIT_STATUS status --porcelain)
read_git_value(SAMPLE_GIT_NEAREST_TAG describe --tags --always --long)
read_git_value(SAMPLE_GIT_TRACKING_BRANCH rev-parse --abbrev-ref --symbolic-full-name @{upstream})
if(SAMPLE_GIT_BRANCH STREQUAL "Not available")
    set(SAMPLE_GIT_HEAD_DETACHED "true")
else()
    set(SAMPLE_GIT_HEAD_DETACHED "false")
endif()
if(SAMPLE_GIT_STATUS STREQUAL "Not available")
    set(SAMPLE_GIT_STATUS "clean")
endif()
execute_process(COMMAND git rev-list --left-right --count HEAD...@{upstream}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE tracking_result
    OUTPUT_VARIABLE tracking_counts
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(tracking_result EQUAL 0 AND tracking_counts MATCHES "^([0-9]+)[ \t]+([0-9]+)$")
    set(SAMPLE_GIT_TRACKING_AHEAD "${CMAKE_MATCH_1}")
    set(SAMPLE_GIT_TRACKING_BEHIND "${CMAKE_MATCH_2}")
else()
    set(SAMPLE_GIT_TRACKING_AHEAD "Not available")
    set(SAMPLE_GIT_TRACKING_BEHIND "Not available")
endif()
set(SAMPLE_GIT_FILES_URL "Not available")
if(SAMPLE_GIT_REPOSITORY MATCHES "^(https://github\\.com/|git@github\\.com:)(.+)$")
    set(github_path "${CMAKE_MATCH_2}")
    string(REGEX REPLACE "\\.git$" "" github_path "${github_path}")
    set(SAMPLE_GIT_FILES_URL "https://github.com/${github_path}/tree/${SAMPLE_GIT_COMMIT_HASH}")
endif()
# Reconfigure when the external geometry file changes so its new hash is recorded.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h")
file(SHA256 "${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h" SAMPLE_TARGETS_SHA256)
configure_file(src/support/Version.h.in generated/Version.h @ONLY)
# endregion

# Build graph ------------------------------------------------------------------------------------------------------------------------------------------------------------

# region Build graph
# Purpose: Add source directories in the order needed by their production targets.
include(CTest)
add_subdirectory(src)
# endregion

# Installation -----------------------------------------------------------------------------------------------------------------------------------------------------------

# region Installation
# Purpose: Install shared configuration resources; each workflow source tree installs its own programs.
install(DIRECTORY config/ DESTINATION share/clas12-samples/config)
# endregion
