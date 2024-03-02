add_executable(solver_tests test_solver.cpp)
add_dependencies(solver_tests oRatioLib)
target_link_libraries(solver_tests PRIVATE oRatioLib)

add_test(NAME oRatio_LibTest COMMAND solver_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})