cmake_minimum_required(VERSION 3.5.0)
project(oRatioLib VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

set(JSON_INCLUDE_UTILS OFF CACHE BOOL "Enable JSON Utils" FORCE)

add_subdirectory(extern/riddle)
add_subdirectory(extern/semitone)

set(TEMPORAL_NETWORK_TYPES LRA DL)
set(TEMPORAL_NETWORK_TYPE LRA CACHE STRING "Temporal network type")
set_property(CACHE TEMPORAL_NETWORK_TYPE PROPERTY STRINGS ${TEMPORAL_NETWORK_TYPES})
list(FIND TEMPORAL_NETWORK_TYPES ${TEMPORAL_NETWORK_TYPE} TEMPORAL_NETWORK_TYPE_INDEX)
if(${TEMPORAL_NETWORK_TYPE_INDEX} EQUAL -1)
    message(FATAL_ERROR "Invalid temporal network type ${TEMPORAL_NETWORK_TYPE}")
endif()
message(STATUS "Temporal network type: ${TEMPORAL_NETWORK_TYPE}")

add_library(oRatioLib src/solver.cpp src/solver_item.cpp src/graph.cpp src/flaw.cpp src/resolver.cpp)
target_link_libraries(oRatioLib PUBLIC RiDDLe SeMiTONE)
target_include_directories(oRatioLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

if(${TEMPORAL_NETWORK_TYPE} STREQUAL "LRA")
    set(INIT_STRING "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end) { start >= origin; start <= end; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;" CACHE STRING "Initialization string")
    target_compile_definitions(oRatioLib PRIVATE LRA_TN)
elseif(${TEMPORAL_NETWORK_TYPE} STREQUAL "DL")
    set(INIT_STRING "predicate Impulse(time at) { at >= origin; at <= horizon; } predicate Interval(time start, time end) { start >= origin; start <= end; end <= horizon; } time origin, horizon; origin >= 0.0; origin <= horizon;" CACHE STRING "Initialization string")
    target_compile_definitions(oRatioLib PRIVATE DL_TN)
endif()
configure_file(include/init.hpp.in init.hpp @ONLY)

add_executable(oRatio src/main.cpp)
target_link_libraries(oRatio PRIVATE oRatioLib)
target_include_directories(oRatio PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
