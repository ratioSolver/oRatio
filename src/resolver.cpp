#include "resolver.h"
#include "flaw.h"
#include "solver.h"

namespace ratio
{
    resolver::resolver(flaw &f, const utils::rational &cost) : resolver(f, semitone::lit(f.s.sat->new_var()), cost) {}
    resolver::resolver(flaw &f, const semitone::lit &rho, const utils::rational &cost) : f(f), rho(rho), intrinsic_cost(cost) {}
} // namespace ratio