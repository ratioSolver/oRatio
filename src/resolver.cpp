#include "resolver.hpp"
#include "flaw.hpp"

namespace ratio
{
    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) : f(f), intrinsic_cost(intrinsic_cost)
    {
        f.resolvers.push_back(*this);
    }
} // namespace ratio