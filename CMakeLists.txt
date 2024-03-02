cmake_minimum_required(VERSION 3.5.0)
project(oRatio VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

add_subdirectory(extern/riddle)
add_subdirectory(extern/semitone)

add_library(oRatio src/solver.cpp)
target_link_libraries(oRatio PUBLIC RiDDLe SeMiTONE)
target_include_directories(oRatio PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
