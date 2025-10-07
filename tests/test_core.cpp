#include "solver.hpp"
#include <cassert>

void test_basic_core()
{
    ratio::solver slv;

    // auto i0 = slv.new_int();
    // auto i1 = slv.new_int();
    // auto i2 = slv.new_int();

    // slv.new_clause({slv.new_eq(i0, i1)});
    // slv.new_clause({slv.new_ge(i1, slv.new_int(10))});

    // slv.solve();

    // assert(slv.arith_value(*i0) >= 10);
    // assert(slv.arith_value(*i1) == slv.arith_value(*i0));
    // assert(slv.arith_value(*i2) == 0);
}

int main()
{
    test_basic_core();

    return 0;
}
