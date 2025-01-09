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

    z3resolver::z3resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : resolver(f, std::move(intrinsic_cost)), rho(static_cast<z3solver &>(f.get_graph()).ctx.bool_const(("b" + std::to_string(static_cast<z3solver &>(f.get_graph()).bool_count++)).c_str())) {}
    z3resolver::z3resolver(flaw &f, utils::rational &&intrinsic_cost, z3::expr &&rho) noexcept : resolver(f, std::move(intrinsic_cost)), rho(std::move(rho)) {}

    z3atom_flaw::z3atom_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args) noexcept : z3flaw(slv, std::move(causes)), atm(std::make_shared<atom>(*this, pred, is_fact, std::move(args))) {}

    void z3atom_flaw::compute_resolvers()
    {
        for (auto unf_atm : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (unf_atm.get() != atm.get() && static_cast<atom *>(unf_atm.get())->get_flaw().is_expanded())
                new_resolver<z3unify_atom>(*this, unf_atm);

        if (atm->is_fact())
            if (get_resolvers().empty())
                new_resolver<z3activate_fact>(*this, z3::expr(get_phi()));
            else
                new_resolver<z3activate_fact>(*this);
        else if (get_resolvers().empty())
            new_resolver<z3activate_goal>(*this, z3::expr(get_phi()));
        else
            new_resolver<z3activate_goal>(*this);
    }

    z3activate_fact::z3activate_fact(z3atom_flaw &f) noexcept : z3resolver(f, utils::rational(1)) {}
    z3activate_fact::z3activate_fact(z3atom_flaw &f, z3::expr &&rho) noexcept : z3resolver(f, utils::rational(1), std::move(rho)) {}
    void z3activate_fact::apply()
    {
    }

    z3activate_goal::z3activate_goal(z3atom_flaw &f) noexcept : z3resolver(f, utils::rational(1)) {}
    z3activate_goal::z3activate_goal(z3atom_flaw &f, z3::expr &&rho) noexcept : z3resolver(f, utils::rational(1), std::move(rho)) {}
    void z3activate_goal::apply()
    {
    }

    z3unify_atom::z3unify_atom(z3atom_flaw &f, riddle::atom_expr atm) noexcept : z3resolver(f, utils::rational(1)), atm(atm) {}
    void z3unify_atom::apply()
    {
    }

    z3disjunction_flaw::z3disjunction_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : z3flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void z3disjunction_flaw::compute_resolvers()
    {
    }

    z3choose_conjunction::z3choose_conjunction(z3disjunction_flaw &f, riddle::conjunction &conj) noexcept : z3resolver(f, utils::rational(1)), conj(conj) {}
} // namespace ratio