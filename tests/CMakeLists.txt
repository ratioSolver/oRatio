add_executable(core_tests test_core.cpp)
add_dependencies(core_tests oRatioLib)
target_link_libraries(core_tests PRIVATE oRatioLib)

add_executable(solver_tests test_solver.cpp)
add_dependencies(solver_tests oRatioLib)
target_compile_definitions(solver_tests PRIVATE NUM_TESTS=1)
target_link_libraries(solver_tests PRIVATE oRatioLib)

add_test(NAME oRatio_CoreTest COMMAND core_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

if(TEMPORAL_NETWORK_TYPE STREQUAL LRA)
    add_test(NAME SolverTest00 COMMAND solver_tests "${PROJECT_SOURCE_DIR}/extern/riddle/examples/core/example_00.rddl" "solution.json")
elseif(TEMPORAL_NETWORK_TYPE STREQUAL DL)
endif()