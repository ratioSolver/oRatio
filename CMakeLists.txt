cmake_minimum_required(VERSION 3.5.0)
project(oRatioLib VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

include(CTest)
enable_testing()

set(BUILD_LISTENERS ON CACHE BOOL "Builds oRatio listeners" FORCE)

option(CHECK_INCONSISTENCIES "Check inconsistencies at each step" OFF)

set(TEMPORAL_NETWORK_TYPES LRA DL)
set(TEMPORAL_NETWORK_TYPE LRA CACHE STRING "Temporal network type")
set_property(CACHE TEMPORAL_NETWORK_TYPE PROPERTY STRINGS ${TEMPORAL_NETWORK_TYPES})
list(FIND TEMPORAL_NETWORK_TYPES ${TEMPORAL_NETWORK_TYPE} TEMPORAL_NETWORK_TYPE_INDEX)
if(${TEMPORAL_NETWORK_TYPE_INDEX} EQUAL -1)
    message(FATAL_ERROR "Invalid temporal network type ${TEMPORAL_NETWORK_TYPE}")
endif()
message(STATUS "Temporal network type: ${TEMPORAL_NETWORK_TYPE}")

set(HEURISTIC_TYPES h_max h_add)
set(HEURISTIC_TYPE h_max CACHE STRING "Heuristic type")
set_property(CACHE HEURISTIC_TYPE PROPERTY STRINGS ${HEURISTIC_TYPES})
list(FIND HEURISTIC_TYPES ${HEURISTIC_TYPE} HEURISTIC_TYPE_INDEX)
if(${HEURISTIC_TYPE_INDEX} EQUAL -1)
    message(FATAL_ERROR "Invalid heuristic type ${HEURISTIC_TYPE}")
endif()
message(STATUS "Heuristic type: ${HEURISTIC_TYPE}")

add_subdirectory(extern/riddle)

add_library(oRatioLib src/solver.cpp)
target_compile_features(oRatioLib PUBLIC cxx_std_17)
add_dependencies(oRatioLib RiDDLe)
target_link_libraries(oRatioLib PUBLIC RiDDLe)
target_include_directories(oRatioLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(${HEURISTIC_TYPE} STREQUAL "h_max")
    target_compile_definitions(oRatioLib PRIVATE H_MAX)
elseif(${HEURISTIC_TYPE} STREQUAL "h_add")
    target_compile_definitions(oRatioLib PRIVATE H_ADD)
endif()

message(STATUS "Check inconsistencies at each step: ${CHECK_INCONSISTENCIES}")
if(CHECK_INCONSISTENCIES)
    target_compile_definitions(oRatioLib PRIVATE CHECK_INCONSISTENCIES)
endif()

message(STATUS "Build oRatio listeners: ${BUILD_LISTENERS}")
if(BUILD_LISTENERS)
    target_compile_definitions(oRatioLib PUBLIC BUILD_LISTENERS)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
