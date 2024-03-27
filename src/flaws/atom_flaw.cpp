#include <cassert>
#include "atom_flaw.hpp"
#include "solver.hpp"
#include "sat_core.hpp"

namespace ratio
{
    atom_flaw::atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept : flaw(s, std::move(causes)), atm(std::make_shared<atom>(pred, *this, is_fact, utils::lit(s.get_sat().new_var()), std::move(arguments))) {}
} // namespace ratio
