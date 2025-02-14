#if defined(SEMITONE)
#include "stsolver.hpp"
#elif defined(MathSAT)
#include "msatsolver.hpp"
#elif defined(Z3)
#include "z3solver.hpp"
#endif
#include <cassert>

void test_basic_core()
{
#if defined(SEMITONE)
    ratio::stsolver slv;
#elif defined(MathSAT)
    ratio::msatsolver slv;
#elif defined(Z3)
    ratio::z3solver slv;
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
