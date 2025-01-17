#include "z3flaws.hpp"
#include "conjunction.hpp"

namespace ratio
{
    z3flaw::z3flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept : flaw(slv, std::move(causes)), phi(compute_phi(slv, get_causes())), pos(slv.ctx.real_const(("p" + std::to_string(slv.position_count++)).c_str()))
    {
        for (const auto &cause : causes)
            slv.slv.add(z3::implies(phi, pos <= static_cast<z3flaw &>(cause.get().get_flaw()).pos - 1));
    }

    size_t z3flaw::get_position() const noexcept { return static_cast<const z3solver &>(get_graph()).mdl.eval(pos, true).get_numeral_uint(); }

    z3::expr z3flaw::compute_phi(z3solver &slv, const std::vector<std::reference_wrapper<resolver>> &causes) noexcept
    {
        z3::expr_vector args(slv.ctx);
        for (const auto &cause : causes)
            args.push_back(static_cast<z3resolver &>(cause.get()).get_rho());
        return z3::mk_and(args);
    }

    z3resolver::z3resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : resolver(f, std::move(intrinsic_cost)), rho(static_cast<z3solver &>(f.get_graph()).ctx.bool_const(("b" + std::to_string(static_cast<z3solver &>(f.get_graph()).bool_count++)).c_str())) {}
    z3resolver::z3resolver(flaw &f, utils::rational &&intrinsic_cost, z3::expr &&rho) noexcept : resolver(f, std::move(intrinsic_cost)), rho(std::move(rho)) {}

    void z3resolver::add(const z3::expr &e) { static_cast<z3solver &>(get_flaw().get_graph()).slv.add(z3::implies(rho, e)); }

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
        // activating the resolver means activating the atom..
        add(static_cast<atom &>(*static_cast<z3atom_flaw &>(get_flaw()).get_atom()).get_sigma() == 1);
    }

    z3activate_goal::z3activate_goal(z3atom_flaw &f) noexcept : z3resolver(f, utils::rational(1)) {}
    z3activate_goal::z3activate_goal(z3atom_flaw &f, z3::expr &&rho) noexcept : z3resolver(f, utils::rational(1), std::move(rho)) {}
    void z3activate_goal::apply()
    {
        // activating the resolver means activating the atom..
        add(static_cast<atom &>(*static_cast<z3atom_flaw &>(get_flaw()).get_atom()).get_sigma() == 1);
        // we also call the corresponding rule..
        static_cast<riddle::predicate &>(static_cast<atom &>(*static_cast<z3atom_flaw &>(get_flaw()).get_atom()).get_type()).call(static_cast<z3atom_flaw &>(get_flaw()).get_atom());
    }

    z3unify_atom::z3unify_atom(z3atom_flaw &f, riddle::atom_expr atm) noexcept : z3resolver(f, utils::rational(1)), atm(atm) {}
    void z3unify_atom::apply()
    {
        // unifying the atom means unifying the atoms..
        add(static_cast<atom &>(*static_cast<z3atom_flaw &>(get_flaw()).get_atom()).get_sigma() == 2);
        // ..the unified atom must be active..
        add(static_cast<atom &>(*atm).get_sigma() == 1);
        // ..and the two atoms must be equal..
        auto eq = *static_cast<z3atom_flaw &>(get_flaw()).get_atom() == atm;
        add(static_cast<bool_item &>(*eq).get_expr());
    }

    z3disjunction_flaw::z3disjunction_flaw(z3solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : z3flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void z3disjunction_flaw::compute_resolvers()
    {
        for (auto &disj : disjuncts)
            new_resolver<z3choose_conjunction>(*this, *disj);
    }

    z3choose_conjunction::z3choose_conjunction(z3disjunction_flaw &f, riddle::conjunction &conj) noexcept : z3resolver(f, utils::rational(1)), conj(conj) {}
    void z3choose_conjunction::apply() { conj.execute(); }
} // namespace ratio