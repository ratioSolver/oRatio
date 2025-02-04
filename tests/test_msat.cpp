#include "mathsat.h"
#include <iostream>

int main()
{
    auto cfg = msat_create_config();
    msat_set_option(cfg, "model_generation", "true");
    auto env = msat_create_env(cfg);

    auto int_sort = msat_get_integer_type(env);
    auto x = msat_make_constant(env, msat_declare_function(env, "x", int_sort));
    auto y = msat_make_constant(env, msat_declare_function(env, "y", int_sort));
    auto x_plus_y = msat_make_plus(env, x, y);
    auto x_plus_y_eq_10 = msat_make_equal(env, x_plus_y, msat_make_number(env, "10"));

    msat_assert_formula(env, x_plus_y_eq_10);

    auto res = msat_solve(env);
    if (res == MSAT_SAT)
    {
        auto model = msat_get_model(env);
        auto x_val = msat_model_eval(model, x);
        auto y_val = msat_model_eval(model, y);
        std::cout << "x = " << msat_term_repr(x_val) << std::endl;
        std::cout << "y = " << msat_term_repr(y_val) << std::endl;
    }
    else
    {
        std::cout << "Unsat" << std::endl;
    }
    return 0;
}
