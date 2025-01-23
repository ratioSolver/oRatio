#include "c++/z3++.h"
#include "logging.hpp"

void test_z3()
{
    z3::context ctx;
    z3::solver s(ctx);
    z3::expr x = ctx.int_const("x");
    z3::expr y = ctx.int_const("y");
    z3::expr z = ctx.int_const("z");
    s.add(x + y + z == 3);
    s.check();
    z3::model m = s.get_model();
    LOG_INFO("Model: " << m);
}

void test_model()
{
    z3::context ctx;
    z3::solver s(ctx);
    z3::expr x = ctx.int_val(1);
    z3::expr y = ctx.int_const("y");
    z3::expr z = x == y;
    s.check();
    z3::model m = s.get_model();
    LOG_INFO("Model: " << m);
    LOG_INFO("x = " << m.eval(x, true));
    LOG_INFO("y = " << m.eval(y, true));
    LOG_INFO("z = " << m.eval(z, true));
    s.add(z);
    s.check();
    m = s.get_model();
    LOG_INFO("Model: " << m);
    LOG_INFO("x = " << m.eval(x, true));
    LOG_INFO("y = " << m.eval(y, true));
    LOG_INFO("z = " << m.eval(z, true));
}

int main()
{
    test_z3();
    test_model();

    return 0;
}
