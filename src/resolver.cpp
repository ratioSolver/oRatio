#include "resolver.h"
#include "flaw.h"
#include "solver.h"

namespace ratio
{
    resolver::resolver(const flaw &f, const utils::rational &cost) : resolver(f, cost, semitone::lit(f.s.sat->new_var())) {}
    resolver::resolver(const flaw &f, const utils::rational &cost, const semitone::lit &rho) : f(f), intrinsic_cost(cost), rho(rho) {}
} // namespace ratio