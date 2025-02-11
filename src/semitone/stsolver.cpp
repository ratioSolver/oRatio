#include "stsolver.hpp"

namespace ratio
{
    stsolver::stsolver(std::string_view name) noexcept : graph(name) {}
} // namespace ratio