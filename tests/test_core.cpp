#include "solver.h"
#include <cassert>

void test_basic_core()
{
    // we create a solver
    ratio::solver s;

    // we create some variables
    auto x = s.new_real();
    auto y = s.new_real();

    // we create a constraint
    auto eq = s.eq(s.add({x, y}), s.new_real(utils::rational::ONE));

    // we assert the constraint
    s.assert_fact(eq);

    // we solve the problem
    auto res = s.solve();

    // we check the result
    assert(res);
    auto x_val = s.arith_value(x);
    auto y_val = s.arith_value(y);
    assert(x_val + y_val == utils::rational::ONE);

    // we pop the last decision level
    s.get_sat_core().pop();

    // we create a new constraint
    auto x_geq_5 = s.geq(x, s.new_real(utils::rational(5)));

    // we assert the constraint
    s.assert_fact(x_geq_5);

    // we solve the problem
    res = s.solve();

    // we check the result
    assert(res);
    x_val = s.arith_value(x);
    y_val = s.arith_value(y);
    assert(x_val + y_val == utils::rational::ONE);
    assert(x_val >= utils::rational(5));
}

int main(int argc, char const *argv[])
{
    test_basic_core();

    return 0;
}
