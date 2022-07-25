add_executable(solver_tests test_solver.cpp)
target_include_directories(solver_tests PRIVATE ${oRatio_SOURCE_DIR}/smt ${oRatio_SOURCE_DIR}/smt/lra ${oRatio_SOURCE_DIR}/smt/ov ${oRatio_SOURCE_DIR}/smt/dl ${oRatio_SOURCE_DIR}/core ${oRatio_SOURCE_DIR}/solver)
target_link_libraries(solver_tests PRIVATE oRatio ratioCore RiDDLe SeMiTONE)

target_compile_definitions(solver_tests PRIVATE NUM_TESTS=1)