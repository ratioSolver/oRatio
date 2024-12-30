#include "solver.hpp"
#include <cassert>

void test_basic_core()
{
    ratio::solver slv;

    auto i0 = slv.new_int();
    auto i1 = slv.new_int();
    auto i2 = slv.new_int();

    slv.assert_fact(slv.new_eq(i0, i1));

    slv.solve();

    assert(slv.arith_value(*i0) == 0);
    assert(slv.arith_value(*i1) == 0);
    assert(slv.arith_value(*i2) == 0);
}

int main(int argc, char const *argv[])
{
    test_basic_core();

    return 0;
}
