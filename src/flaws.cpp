#include "flaws.hpp"
#include "solver.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    flaw::flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : listener(slv.ac_slv), slv(slv), phi(causes.empty() ? utils::TRUE_lit : slv.ac_slv.new_sat()), causes(std::move(causes)), exclusive(exclusive)
    {
        if (causes.empty()) // if there are no causes, the flaw is a root flaw, so it is active by default..
            slv.active_flaws.insert(this);
        else if (get_state() == utils::Undefined) // otherwise, we need to monitor its state..
            listen_to(utils::variable(phi));
    }
    utils::lbool flaw::get_state() const noexcept { return slv.ac_slv.sat_val(phi); }
    void flaw::on_domain_changed(const utils::var v) noexcept
    {
        assert(utils::variable(phi) == v && "Domain change notified for a variable not associated with this flaw.");
        if (get_state() == utils::True)    // if the flaw is now active..
            slv.active_flaws.insert(this); // add it to the active flaws..
        slv.flaw_state_changed(*this);
    }
    json::json flaw::to_json() const
    {
        json::json j_flaw{{"cost", linspire::to_json(est_cost)}, {"state", to_string(slv.ac_slv.sat_val(phi))}};
        if (!causes.empty())
        {
            json::json j_causes(json::json_type::array);
            for (const auto &c : causes)
                j_causes.push_back(c.get().get_id());
            j_flaw["causes"] = std::move(j_causes);
        }
        return j_flaw;
    }

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : resolver(f, std::move(intrinsic_cost), f.slv.ac_slv.new_sat()) {}
    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept : listener(f.slv.ac_slv), f(f), intrinsic_cost(std::move(intrinsic_cost)), rho(rho)
    {
        f.resolvers.push_back(*this);
        if (get_state() == utils::Undefined) // if the resolver is not yet active or forbidden, we need to monitor its state..
            listen_to(utils::variable(rho));
        f.slv.lin_cnst_to_resolvers[&cnst].push_back(*this);
    }
    utils::rational resolver::get_estimated_cost() const noexcept
    {
        if (get_state() == utils::False)
            return utils::rational::positive_infinite;
        else if (preconditions.empty())
            return intrinsic_cost;
#ifdef H_ADD
        // we compute the cost of the resolver as the sum of its intrinsic cost and the estimated costs of its preconditions..
        return std::accumulate(preconditions.begin(), preconditions.end(), intrinsic_cost, [](const auto &lhs, const auto &prec)
                               { return lhs + prec.get().get_estimated_cost(); });
#endif
#ifdef H_MAX
        // we compute the cost of the resolver as the sum of its intrinsic cost and the maximum of its preconditions' estimated costs..
        return intrinsic_cost + (*std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                   { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); }))
                                    .get()
                                    .get_estimated_cost();
#endif
    }
    utils::lbool resolver::get_state() const noexcept { return f.slv.ac_slv.sat_val(rho); }
    void resolver::retract() noexcept
    {
        f.slv.lin_slv.retract(cnst);
        for (auto &ac_cnst : ac_cnsts)
            f.slv.ac_slv.retract(ac_cnst.get());
    }
    void resolver::on_domain_changed(const utils::var v) noexcept
    {
        assert(utils::variable(rho) == v && "Domain change notified for a variable not associated with this resolver.");
        f.slv.resolver_state_changed(*this);
    }
    bool resolver::execute(const riddle::bool_expr &expr) noexcept { return f.get_solver().execute(expr); }
    void resolver::new_clause(std::vector<utils::lit> &&lits)
    {
        auto &clause = get_solver().ac_slv.new_clause(std::move(lits));
        ac_cnsts.push_back(clause);
        get_solver().ac_slv.add_constraint(clause);
    }
    json::json resolver::to_json() const
    {
        json::json j_resolver{{"flaw", f.get_id()}, {"intrinsic_cost", linspire::to_json(intrinsic_cost)}, {"state", to_string(f.slv.ac_slv.sat_val(rho))}};
        return j_resolver;
    }

    enum_flaw::enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    void enum_flaw::compute_resolvers() {}

    clause_flaw::clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers() {}

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers() {}

    atom_flaw::atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(*this, pred, is_fact, std::move(args), std::move(sigma))) {}

    void atom_flaw::compute_resolvers()
    {
        assert(get_state() != utils::False && "Cannot compute resolvers for a forbidden flaw.");
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
        {
            if (a == atm)
                continue; // skip self..
            if (!static_cast<atom &>(*a).get_flaw().is_expanded())
                continue; // skip not expanded flaws..
            if (can_unify_with(*a))
            {
                LOG_TRACE("  Found unifiable atom: " << a->get_id());
                slv.new_resolver<unify_atom>(*this, a);
            }
        }

        if (atm->is_fact())
            if (get_resolvers().empty())
                slv.new_resolver<activate_fact>(*this, get_phi());
            else
                slv.new_resolver<activate_fact>(*this);
        else if (get_resolvers().empty())
            slv.new_resolver<activate_goal>(*this, get_phi());
        else
            slv.new_resolver<activate_goal>(*this);
    }

    bool atom_flaw::can_unify_with(const riddle::atom_term &other) const noexcept
    {
        assert(atm.get() != &other && "Cannot unify an atom with itself.");
        assert(&atm->get_type() == &other.get_type() && "Cannot unify atoms of different predicates.");
        auto c_f = this;
        while (!c_f->get_causes().empty())
        {
            if (auto c_c = dynamic_cast<activate_fact *>(&c_f->get_causes().front().get()))
                c_f = static_cast<atom_flaw *>(&c_c->get_flaw());
            else if (auto c_c = dynamic_cast<activate_goal *>(&c_f->get_causes().front().get()))
                c_f = static_cast<atom_flaw *>(&c_c->get_flaw());
            assert(c_f && "Unexpected resolver type in cause chain.");
            if (c_f->atm.get() == &other)
                return false; // found a cycle in the unification chain..
        }
        return true;
    }

    json::json atom_flaw::to_json() const
    {
        json::json j_flaw = flaw::to_json();
        j_flaw["data"] = {{"type", "atom"}, {"atom", {{"atom_id", atm->get_id()}, {"is_fact", atm->is_fact()}, {"predicate", atm->get_type().get_name()}, {"sigma", 0}}}};
        return j_flaw;
    }

    activate_fact::activate_fact(atom_flaw &f) noexcept : resolver(f, 1) {}
    activate_fact::activate_fact(atom_flaw &f, const utils::lit &rho) noexcept : resolver(f, 1, rho) {}
    void activate_fact::apply()
    {
        assert(static_cast<atom_flaw &>(get_flaw()).get_atom()->is_fact() && "activate_fact resolver applied to a non-fact atom.");

        // activating this resolver activates the atom associated with the flaw..
        new_clause({!get_rho(), static_cast<atom &>(*static_cast<atom_flaw &>(get_flaw()).get_atom()).get_sigma()});
    }
    json::json activate_fact::to_json() const
    {
        json::json j_res = resolver::to_json();
        j_res["data"] = {{"type", "activate_fact"}};
        return j_res;
    }

    activate_goal::activate_goal(atom_flaw &f) noexcept : resolver(f, 1) {}
    activate_goal::activate_goal(atom_flaw &f, const utils::lit &rho) noexcept : resolver(f, 1, rho) {}
    void activate_goal::apply()
    {
        assert(!static_cast<atom_flaw &>(get_flaw()).get_atom()->is_fact() && "activate_goal resolver applied to a fact atom.");

        // activating this resolver activates the atom associated with the flaw..
        new_clause({!get_rho(), static_cast<atom &>(*static_cast<atom_flaw &>(get_flaw()).get_atom()).get_sigma()});

        // we call the predicate's procedure associated with the atom..
        static_cast<riddle::predicate &>(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_type()).call(static_cast<atom_flaw &>(get_flaw()).get_atom());
    }
    json::json activate_goal::to_json() const
    {
        json::json j_res = resolver::to_json();
        j_res["data"] = {{"type", "activate_goal"}};
        return j_res;
    }

    unify_atom::unify_atom(atom_flaw &f, riddle::atom_expr atm) noexcept : resolver(f, 1), atm(std::move(atm)) {}
    void unify_atom::apply()
    {
        if (execute(get_solver().new_eq(static_cast<atom_flaw &>(get_flaw()).get_atom(), atm)))
        {
            //  - we make the current atom's sigma false (unified atom)..
            new_clause({!get_rho(), !static_cast<atom &>(*static_cast<atom_flaw &>(get_flaw()).get_atom()).get_sigma()});
            //  - and we make the target atom's sigma true (active atom)..
            new_clause({!get_rho(), static_cast<atom &>(*atm).get_sigma()});
        }
    }
    json::json unify_atom::to_json() const
    {
        json::json j_res = resolver::to_json();
        j_res["data"] = {{"type", "unify_atom"}, {"target", atm->get_id()}};
        return j_res;
    }
} // namespace ratio
