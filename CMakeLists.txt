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
option(GRAPH_PRUNING "Prunes the causal graph before starting the search" ON)
option(BUILD_LISTENERS "Builds the solver's listeners" OFF)

add_subdirectory(extern/core)

file(GLOB RATIO_SOURCES src/*.cpp)
file(GLOB RATIO_HEADERS include/*.h)

add_library(${PROJECT_NAME} SHARED ${RATIO_SOURCES})
GENERATE_EXPORT_HEADER(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:>)
target_link_libraries(${PROJECT_NAME} PRIVATE ratioCore RiDDLe SeMiTONE)

message(STATUS "Temporal network type:    ${TEMPORAL_NETWORK_TYPE}")
if(TEMPORAL_NETWORK_TYPE STREQUAL DL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DL_TN)
    set(INIT_STRING "predicate Impulse(tp at) { at >= origin; at <= horizon; } predicate Interval(tp start, tp end) { start >= origin; start <= end; end <= horizon; } tp origin; tp horizon; origin >= 0.0; origin <= horizon;")
elseif(TEMPORAL_NETWORK_TYPE STREQUAL LA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LA_TN)
    set(INIT_STRING "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; end <= horizon; duration == end - start; duration >= 0.0; } real origin; real horizon; origin >= 0.0; origin <= horizon;")
else()
    message(FATAL_ERROR "TEMPORAL_NETWORK_TYPE must be one of ${TEMPORAL_NETWORK_TYPES}")
endif()
configure_file(include/init.h.in init.h @ONLY)

message(STATUS "Prune causal graph:     ${GRAPH_PRUNING}")
if(GRAPH_PRUNING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GRAPH_PRUNING)
endif()

message(STATUS "Build solver listeners: ${BUILD_LISTENERS}")
if(BUILD_LISTENERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_LISTENERS)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    FILES ${RATIO_CORE_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
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
