#include "z3flaws.hpp"

namespace ratio
{
    z3atom_flaw::z3atom_flaw(graph &gr, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::atom_expr atom, z3::expr phi) noexcept : flaw(gr, std::move(causes)), atom(atom), phi(phi) {}
} // namespace ratio