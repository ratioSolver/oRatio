#include <cassert>
#include "resolver.hpp"
#include "solver.hpp"
#include "flaw.hpp"
#include "sat_core.hpp"

namespace ratio
{
    resolver::resolver(flaw &f, const utils::rational &intrinsic_cost) : resolver(f, utils::lit(f.get_solver().get_sat().new_var()), intrinsic_cost) {}
    resolver::resolver(flaw &f, const utils::lit &rho, const utils::rational &intrinsic_cost) : f(f), rho(rho), intrinsic_cost(intrinsic_cost) { assert(f.get_solver().get_sat().value(rho) != utils::False); }
} // namespace ratio
