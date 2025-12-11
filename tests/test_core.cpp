#ifdef BASIC_SOLVER
#include "basic_solver.hpp"
#elif defined(LOCAL_SEARCH_SOLVER)
#include "local_search_solver.hpp"
#endif
#include <cassert>

void test_basic_core()
{
#ifdef BASIC_SOLVER
    ratio::basic_solver slv;
#elif defined(LOCAL_SEARCH_SOLVER)
    ratio::local_search_solver slv;
#endif

    auto i0 = slv.new_int();
    auto i1 = slv.new_int();
    auto i2 = slv.new_int();

    slv.new_clause({slv.new_eq(i0, i1)});
    slv.new_clause({slv.new_ge(i1, slv.new_int(10))});

    slv.solve();

    assert(slv.arith_value(*i0) >= 10);
    assert(slv.arith_value(*i1) == slv.arith_value(*i0));
    assert(slv.arith_value(*i2) == 0);
}

int main()
{
    test_basic_core();

    return 0;
}
