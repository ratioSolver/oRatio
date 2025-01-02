#include "solver.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    solver::solver() : core() {}

    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        throw std::runtime_error("Not implemented");
    }

    riddle::atom_expr solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args)
    {
        auto atm = core::create_atom(is_fact, pred, std::move(args));
        return atm;
    }
} // namespace ratio