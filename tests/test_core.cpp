#include <cassert>
#include "solver.hpp"

void test_basic_core()
{
    auto s = std::make_shared<ratio::solver>();
    s->init();

    // we create some variables
    auto x = s->new_real();
    auto y = s->new_real();

    // we create a constraint
    auto eq = s->eq(s->add({x, y}), s->core::new_real(utils::rational::one));
}

int main(int argc, char const *argv[])
{
    test_basic_core();

    return 0;
}
