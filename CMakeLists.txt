cmake_minimum_required(VERSION 3.0.0)
project(oRatio VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(CTest)
enable_testing()

set(TEMPORAL_NETWORK_TYPES DL LA)
set(TEMPORAL_NETWORK_TYPE LA CACHE STRING "Temporal network type")
set_property(CACHE TEMPORAL_NETWORK_TYPE PROPERTY STRINGS ${TEMPORAL_NETWORK_TYPES})

set(HEURISTIC_TYPES h_max h_add)
set(HEURISTIC_TYPE h_max CACHE STRING "Heuristic type")
set_property(CACHE HEURISTIC_TYPE PROPERTY STRINGS ${HEURISTIC_TYPES})

option(DEFERRABLE_FLAWS "Check for deferrable flaws" ON)
option(GRAPH_PRUNING "Prunes the causal graph before starting the search" ON)
option(CHECK_INCONSISTENCIES "Check inconsistencies at each step" OFF)
option(BUILD_LISTENERS "Builds the solver's listeners" OFF)

add_subdirectory(extern/core)

file(GLOB RATIO_SOURCES src/*.cpp src/flaws/*.cpp src/types/*.cpp src/heuristics/*.cpp)
file(GLOB RATIO_HEADERS include/*.h include/flaws/*.h include/types/*.h include/heuristics/*.h)

add_library(${PROJECT_NAME} SHARED ${RATIO_SOURCES})
GENERATE_EXPORT_HEADER(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include/flaws $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include/types $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include/heuristics $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:>)
target_link_libraries(${PROJECT_NAME} PRIVATE ratioCore RiDDLe SeMiTONE json)

message(STATUS "Temporal network type:  ${TEMPORAL_NETWORK_TYPE}")
if(TEMPORAL_NETWORK_TYPE STREQUAL DL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DL_TN)
    set(INIT_STRING "predicate Impulse(time at) { at >= origin; at <= horizon; } predicate Interval(time start, time end) { start >= origin; start <= end; end <= horizon; } time origin; time horizon; origin >= 0.0; origin <= horizon;")
elseif(TEMPORAL_NETWORK_TYPE STREQUAL LA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LA_TN)
    set(INIT_STRING "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; end <= horizon; duration == end - start; duration >= 0.0; } real origin; real horizon; origin >= 0.0; origin <= horizon;")
else()
    message(FATAL_ERROR "TEMPORAL_NETWORK_TYPE must be one of ${TEMPORAL_NETWORK_TYPES}")
endif()
configure_file(include/init.h.in init.h @ONLY)

message(STATUS "Heuristic type:         ${HEURISTIC_TYPE}")
if(HEURISTIC_TYPE STREQUAL h_max)
    target_compile_definitions(${PROJECT_NAME} PRIVATE H_MAX)
elseif(HEURISTIC_TYPE STREQUAL h_add)
    target_compile_definitions(${PROJECT_NAME} PRIVATE H_ADD)
else()
    message(FATAL_ERROR "HEURISTIC_TYPE must be one of ${HEURISTIC_TYPES}")
endif()

message(STATUS "Deferrable flaws:       ${DEFERRABLE_FLAWS}")
if(DEFERRABLE_FLAWS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFERRABLE_FLAWS)
endif()

message(STATUS "Prune graph:            ${GRAPH_PRUNING}")
if(GRAPH_PRUNING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GRAPH_PRUNING)
endif()

message(STATUS "Check inconsistencies:  ${CHECK_INCONSISTENCIES}")
if(CHECK_INCONSISTENCIES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CHECK_INCONSISTENCIES)
endif()

message(STATUS "Build solver listeners: ${BUILD_LISTENERS}")
if(BUILD_LISTENERS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_LISTENERS)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    FILES ${RATIO_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/oratio_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)
configure_package_config_file(src/${PROJECT_NAME}Config.cmake.in ${PROJECT_NAME}Config.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME} PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake VERSION 1.0.0 COMPATIBILITY SameMajorVersion)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
