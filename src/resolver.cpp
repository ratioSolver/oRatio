#include "resolver.h"
#include "flaw.h"

namespace ratio
{
    resolver::resolver(const flaw &f, const utils::rational &cost) : f(f), intrinsic_cost(cost) {}
} // namespace ratio