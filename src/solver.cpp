#include "solver.hpp"
#include "init.hpp"
#include "logging.hpp"

namespace ratio
{
    solver::solver(const std::string &name, bool i) noexcept : name(name), gr(*this)
    {
        if (i)
            init();
    }

    void solver::init() noexcept
    {
        LOG_INFO("Initializing solver " << name);
    }
} // namespace ratio