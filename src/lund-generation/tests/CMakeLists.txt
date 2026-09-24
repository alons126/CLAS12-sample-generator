#
# Created by Alon Sportes on 14/09/2026.
#

# src/lund-generation/tests/CMakeLists.txt -------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Test executables, isolated references and CTest registration.
#
# Purpose:
#   Check generated data, legacy parity, sampling distributions and workflow command contracts.
#
# Workflow:
#   Root enables BUILD_TESTING -> build fixtures/references -> CTest runs registered checks.
#
# Inputs:
#   Python 3.9+, enabled workflow targets, ROOT; csh/tcsh is optional for the launcher test.
#
# Outputs:
#   Test executables, generated fixtures in the build tree and named CTest cases.
#
# Notes:
#   Protected legacy and external sources are read-only. Reference adapters write separate build-tree files; no real server jobs are submitted.

# Test tools and GST fixture ---------------------------------------------------------------------------------------------------------------------------------------------
# region Test tools and GST fixture
# Purpose:
#     Discover Python and build numerical comparison and synthetic-input helpers.
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
if(BUILD_UNIFORM)
    add_executable(compare-histograms compare_histograms.cpp)
    target_link_libraries(compare-histograms PRIVATE ROOT::RIO ROOT::Hist)
    add_executable(check-monitoring check_monitoring.cpp)
    target_link_libraries(check-monitoring PRIVATE ROOT::RIO ROOT::Hist)
endif()
if(BUILD_GENIE)
    add_executable(make-gst-fixture make_gst_fixture.cpp)
    target_link_libraries(make-gst-fixture PRIVATE ROOT::Tree ROOT::RIO)
endif()
# endregion

# Output integration tests -----------------------------------------------------------------------------------------------------------------------------------------------
# region Output integration tests
# Purpose:
#     Test enabled LUND applications and their output contracts.
if(BUILD_UNIFORM)
    add_test(NAME uniform-integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/integration.py uniform $<TARGET_FILE:uniform-lund-generator> $<TARGET_FILE:check-monitoring>)
endif()
if(BUILD_GENIE)
    add_test(NAME genie-gst-integration COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/integration.py genie-gst $<TARGET_FILE:event-generator-to-lund-converter> $<TARGET_FILE:make-gst-fixture>)
endif()
# endregion

# Independent legacy references ------------------------------------------------------------------------------------------------------------------------------------------
# region Independent legacy references
# Purpose:
#     Compile controlled adapters separately from production; compare bytes, bins and distributions.
# Archived implementations are linked only into independent reference executables.
if(BUILD_UNIFORM)
    add_executable(legacy-uniform-reference legacy_uniform_driver.cpp)
    target_include_directories(legacy-uniform-reference PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(legacy-uniform-reference PRIVATE ROOT::Core ROOT::Physics ROOT::Hist ROOT::RIO ROOT::Gpad ROOT::Tree)
    add_test(NAME uniform-legacy-parity COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/legacy_parity.py uniform $<TARGET_FILE:uniform-lund-generator> $<TARGET_FILE:legacy-uniform-reference> $<TARGET_FILE:compare-histograms>)
    add_test(NAME uniform-distributions COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/distributions.py $<TARGET_FILE:uniform-lund-generator>)
endif()
if(BUILD_GENIE)
    set(LEGACY_CONVERTER ${PROJECT_SOURCE_DIR}/legacy/GEMC-samples/GENIE_to_LUND_converter/GENIE_to_LUND_converter.C)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/legacy_genie_reference.cpp
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/prepare_legacy_genie.py ${LEGACY_CONVERTER} ${CMAKE_CURRENT_BINARY_DIR}/legacy_genie_reference.cpp
        DEPENDS ${LEGACY_CONVERTER} ${CMAKE_CURRENT_SOURCE_DIR}/prepare_legacy_genie.py VERBATIM)
    add_executable(legacy-genie-reference ${CMAKE_CURRENT_BINARY_DIR}/legacy_genie_reference.cpp)
    target_link_libraries(legacy-genie-reference PRIVATE ROOT::Core ROOT::Physics ROOT::Tree ROOT::Hist ROOT::RIO ROOT::Gpad ROOT::Graf)
    add_test(NAME genie-gst-legacy-parity COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/legacy_parity.py genie-gst $<TARGET_FILE:event-generator-to-lund-converter> $<TARGET_FILE:legacy-genie-reference> $<TARGET_FILE:make-gst-fixture>)
endif()
# endregion

# Isolated geometry fixture ----------------------------------------------------------------------------------------------------------------------------------------------
# region Isolated geometry fixture
# Purpose:
#     Read the protected header into a separate test fixture; compile the actual adapter against that fixture.
# Prove that replacing the external header alone changes the production adapter.
set(REPLACEMENT_TARGETS ${CMAKE_CURRENT_BINARY_DIR}/replacement/external/targets.h)
add_custom_command(OUTPUT ${REPLACEMENT_TARGETS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/prepare_replacement_geometry.py
        ${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h ${REPLACEMENT_TARGETS}
    DEPENDS ${PROJECT_SOURCE_DIR}/src/lund-generation/external/targets.h ${CMAKE_CURRENT_SOURCE_DIR}/prepare_replacement_geometry.py VERBATIM)
add_executable(replacement-geometry replacement_geometry.cpp ${PROJECT_SOURCE_DIR}/src/lund-generation/core/geometry/TargetGeometry.cpp ${REPLACEMENT_TARGETS})
target_include_directories(replacement-geometry PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/replacement ${PROJECT_SOURCE_DIR}/src/lund-generation)
target_link_libraries(replacement-geometry PRIVATE ROOT::Core ROOT::Physics Threads::Threads)
add_test(NAME replacement-geometry COMMAND replacement-geometry)
# endregion
