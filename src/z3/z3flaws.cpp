#include "z3flaws.hpp"
#include "conjunction.hpp"

namespace ratio
{
    z3flaw::z3flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept : flaw(slv, std::move(causes)), phi(compute_phi(slv, get_causes())) {}

    z3::expr z3flaw::compute_phi(z3solver &slv, const std::vector<std::reference_wrapper<resolver>> &causes) noexcept
    {
        z3::expr_vector args(slv.ctx);
        for (const auto &cause : causes)
            args.push_back(static_cast<z3resolver &>(cause.get()).get_rho());
        return z3::mk_and(args);
    }

    z3resolver::z3resolver(flaw &f, utils::rational &&intrinsic_cost, z3::expr &&rho) noexcept : resolver(f, std::move(intrinsic_cost)), rho(std::move(rho)) {}

    z3atom_flaw::z3atom_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::atom_expr atom) noexcept : z3flaw(slv, std::move(causes)), atom(std::move(atom)) {}

    void z3atom_flaw::compute_resolvers()
    {
    }

    z3disjunction_flaw::z3disjunction_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : z3flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void z3disjunction_flaw::compute_resolvers()
    {
    }
} // namespace ratio