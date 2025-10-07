cmake_minimum_required(VERSION 3.21)
project(oRatioLib VERSION 0.3.0 LANGUAGES CXX)

include(CTest)
enable_testing()

option(ENABLE_API "Enable the oRatio API" OFF)
option(BUILD_EXECUTABLE "Build the oRatio executable" ON)
option(BUILD_ORATIO_SERVER "Build the oRatio server" OFF)

if(BUILD_ORATIO_SERVER)
    set(BUILD_EXECUTABLE ON CACHE BOOL "Build the oRatio executable" FORCE)
    set(ENABLE_API ON CACHE BOOL "Enable the oRatio API" FORCE)
    set(COMPUTE_NAMES ON CACHE BOOL "Compute RiDDLe names" FORCE)
endif()

message(STATUS "Enable the oRatio API: ${ENABLE_API}")
message(STATUS "Build the oRatio executable: ${BUILD_EXECUTABLE}")
message(STATUS "Build the oRatio server: ${BUILD_ORATIO_SERVER}")

add_subdirectory(extern/riddle)
add_subdirectory(extern/linspire)

add_library(oRatioLib src/solver.cpp)
target_compile_features(oRatioLib PUBLIC cxx_std_17)
add_dependencies(oRatioLib RiDDLe LinSpire)
target_link_libraries(oRatioLib PUBLIC RiDDLe LinSpire)
target_include_directories(oRatioLib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

if(ENABLE_API)
    target_compile_definitions(oRatioLib PRIVATE ENABLE_API)
endif()

if (BUILD_EXECUTABLE)
    add_executable(oRatio src/main.cpp)
    add_dependencies(oRatio oRatioLib)
    target_link_libraries(oRatio PUBLIC oRatioLib)

    if(BUILD_ORATIO_SERVER)
        target_compile_definitions(oRatio PRIVATE BUILD_SERVER)
        target_sources(oRatio PRIVATE src/server/solver_server.cpp)
        target_include_directories(oRatio PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/server>)
        if(NOT TARGET ratioNet)
            add_subdirectory(extern/rationet)
        endif()
        add_dependencies(oRatio ratioNet)
        target_link_libraries(oRatio PUBLIC ratioNet)

        add_custom_target(npm_solver_install ALL COMMAND npm install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/solver COMMENT "Installing the oRatio Solver library" VERBATIM)
        add_custom_target(npm_solver_build ALL COMMAND npm run build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/solver COMMENT "Building the oRatio Solver library" VERBATIM DEPENDS npm_solver_install)
        add_custom_target(npm_app_install ALL COMMAND npm install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/app COMMENT "Installing the oRatio WebApp" VERBATIM DEPENDS npm_solver_build)
        add_custom_target(npm_app_build ALL COMMAND npm run build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui/app COMMENT "Building the oRatio WebApp" VERBATIM DEPENDS npm_app_install)
    endif()

    setup_sanitizers(oRatio)
endif()

setup_sanitizers(oRatioLib)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
