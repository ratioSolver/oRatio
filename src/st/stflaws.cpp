#include "stflaws.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace ratio
{
    stflaw::stflaw(solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), prop_listener(slv), dl_listener(slv.get_difference_logic_theory()), phi(compute_phi(slv, get_causes())), pos(slv.mk_tp())
    {
        // this flaw's position must be greater than 0..
        slv.add_distance(pos, 0, utils::rational::zero);
        for (const auto &cause : causes) // we impose the position constraint (i.e., the flaw must be before its causes) to avoid causality loops..
            slv.add_distance(static_cast<stflaw &>(cause->get_flaw()).get_pos(), pos, -utils::rational::one);

        if (get_solver().value(phi) == utils::True) // if the flaw is active, we add it to the set of active flaws..
            set_state(utils::True);
        else // otherwise, we listen to the activation literal..
            listen(variable(phi));
        listen_tp(pos);
    }

    utils::lit stflaw::compute_phi(solver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept
    {
        utils::lit phi;
        if (causes.empty()) // if the flaw has no causes, it is always active..
            phi = utils::TRUE_lit;
        else if (causes.size() == 1) // if the flaw has a single cause, we use its activation literal..
            phi = static_cast<stresolver &>(*causes.front()).get_rho();
        else
        { // we create a new variable for the flaw..
            phi = utils::lit(slv.mk_var());
            std::vector<utils::lit> ls;
            for (const auto &cause : causes)
                ls.push_back(!static_cast<stresolver &>(*cause).get_rho());
            ls.push_back(phi);
            slv.add_clause(std::move(ls));
        }
        return phi;
    }

    void stflaw::expanded_flaw()
    {
        std::vector<utils::lit> ls;
        for (const auto &resolver : get_resolvers())
            ls.push_back(static_cast<stresolver &>(*resolver).get_rho());
        if (get_solver().value(phi) == utils::True)
        {
            if (is_exclusive())
                for (size_t i = 0; i < ls.size(); ++i)
                    for (size_t j = i + 1; j < ls.size(); ++j)
                        get_solver().add_clause({!ls[i], !ls[j]});
            get_solver().add_clause(std::move(ls));
        }
        else
        {
            if (is_exclusive())
                for (size_t i = 0; i < ls.size(); ++i)
                    for (size_t j = i + 1; j < ls.size(); ++j)
                        get_solver().add_clause({!phi, !ls[i], !ls[j]});
            ls.push_back(!phi);
            get_solver().add_clause(std::move(ls));
        }
    }

    void stflaw::on_change(const utils::var &v) noexcept { get_solver().set_flaw_state(*this, get_solver().value(v)); }
    void stflaw::on_reset(const utils::var &v) noexcept { get_solver().set_flaw_state(*this, get_solver().value(v), true); }
    void stflaw::on_tp_change(const utils::var &v) noexcept { get_solver().set_flaw_position(*this, get_solver().tp_bounds(v).first.numerator()); }

    json::json stflaw::to_json() const
    {
        json::json j = flaw::to_json();
        j["phi"] = to_string(phi).c_str();
        j["pos"] = static_cast<uint64_t>(pos);
        return j;
    }

    stresolver::stresolver(stflaw &f, utils::rational &&intrinsic_cost) noexcept : stresolver(f, std::move(intrinsic_cost), utils::lit(f.get_solver().mk_var())) {}
    stresolver::stresolver(stflaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept : resolver(f, std::move(intrinsic_cost)), prop_listener(f.get_solver()), rho(rho)
    {
        assert(get_solver().value(rho) != utils::False);
        get_solver().add_clause({!rho, static_cast<stflaw &>(f).get_phi()});
        if (get_solver().value(rho) == utils::True) // if the resolver is active, the flaw is solved..
            set_state(utils::True);
        else // otherwise, we listen to the activation literal..
            listen(variable(rho));
    }

    void stresolver::on_change(const utils::var &v) noexcept
    {
        get_solver().set_resolver_state(*this, get_solver().value(v));

        // we check if this resolver is mutex with the current resolver..
        if (get_solver().visiting && get_state() == utils::False && get_flaw().get_state() == utils::True)
        { // this flaw is mutex with the current resolver..
            auto cr = get_solver().get_current_resolver().value();
            if (&cr->get_flaw() == &get_flaw())
                return; // the resolvers solve the same flaw, so we ignore the mutex..
            auto dist = get_solver().tp_distance(static_cast<stflaw &>(cr->get_flaw()).get_pos(), static_cast<stflaw &>(get_flaw()).get_pos());
            LOG_DEBUG("[" << to_string(dist.first) << ", " << to_string(dist.second) << "]");
            if (dist.first > 0)
                return; // the resolvers are not mutex..
            if (get_solver().mutexes.count({this, &*cr}) == 0)
            {
                LOG_DEBUG("[" << get_solver().get_name() << "] " << cr->to_json() << " is mutex with " << to_json());
                get_solver().pending_mutexes.emplace_back(this, &*cr);
                get_solver().mutexes.insert({this, &*cr});
                get_solver().mutexes.insert({&*cr, this});
            }
        }
    }
    void stresolver::on_reset(const utils::var &v) noexcept { get_solver().set_resolver_state(*this, get_solver().value(v), true); }

    [[nodiscard]] json::json stresolver::to_json() const
    {
        json::json j = resolver::to_json();
        j["rho"] = to_string(rho).c_str();
        return j;
    }

    enum_flaw::enum_flaw(solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, riddle::component_type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) noexcept : stflaw(slv, std::move(causes), true), var(create_var(tp, std::move(values))) {}
    utils::s_ptr<riddle::enum_item> enum_flaw::create_var(riddle::component_type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        std::vector<utils::lit> lits;
        if (values.size() == 1)
            lits.push_back(utils::TRUE_lit);
        else
            for (size_t i = 0; i < values.size(); i++)
                lits.push_back(utils::lit(static_cast<solver &>(tp.get_scope().get_core()).mk_var()));
        return utils::make_s_ptr<enum_item>(tp, std::move(values), std::move(lits));
    }
    void enum_flaw::compute_resolvers()
    {
        assert(get_solver().value(get_phi()) == get_state());
        for (const auto &val : var->get_values())
            if (get_solver().value(var->get_lit(*val)) != utils::False) // we prune unassignable values..
                new_resolver<choose_val>(*this, *val);
    }

    choose_val::choose_val(enum_flaw &f, const utils::enum_val &val) noexcept : stresolver(f, utils::rational(1), f.get_var()->get_lit(val)), val(val) {}
    void choose_val::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False); // The resolver cannot be negated..
    }

    clause_flaw::clause_flaw(solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause, const bool &exclusive) noexcept : stflaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}
    void clause_flaw::compute_resolvers()
    {
        assert(get_solver().value(get_phi()) == get_state());
        for (const auto &lit : clause)
            if (get_solver().value(lit) != utils::False) // we prune false literals..
                new_resolver<choose_lit>(*this, lit);
    }

    choose_lit::choose_lit(clause_flaw &f, const utils::lit &conj) noexcept : stresolver(f, utils::rational(1), conj) {}
    void choose_lit::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False); // The resolver cannot be negated..
    }

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept : stflaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}
    void disjunction_flaw::compute_resolvers()
    {
        assert(get_solver().value(get_phi()) == get_state());
        for (const auto &disjunct : disjuncts)
            new_resolver<choose_conjunction>(*this, *disjunct);
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &f, riddle::conjunction &conj) noexcept : stresolver(f, utils::rational(1)), conj(conj) {}
    void choose_conjunction::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False); // The resolver cannot be negated..
        conj.execute();
    }

    atom_flaw::atom_flaw(solver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) noexcept : stflaw(slv, std::move(causes)), atm(utils::make_s_ptr<atom>(*this, pred, is_fact, std::move(args), utils::lit(slv.mk_var()))) {}
    void atom_flaw::compute_resolvers()
    {
        assert(get_solver().value(get_phi()) == get_state());
        assert(get_solver().value(atm->get_sigma()) != utils::False); // The atom is not necessarily inactive
        LOG_TRACE("Computing resolvers for " << to_json());
        if (get_solver().value(atm->get_sigma()) == utils::Undefined)
            for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            {
                if (a == atm)
                    continue; // the current atom cannot unify with itself..
                if (!static_cast<atom &>(*a).get_flaw().is_expanded())
                    continue; // the atom is not expanded yet, so we cannot unify it..
                if (get_solver().tp_distance(get_pos(), static_cast<atom &>(*a).get_flaw().get_pos()).first > 0)
                    continue; // the unification would introduce a causal loop..
                if (get_solver().value(static_cast<atom &>(*a).get_sigma()) == utils::False)
                    continue; // the atom is unified with another atom..
                if (get_solver().value(static_cast<atom &>(*a).get_flaw().get_phi()) == utils::False)
                    continue; // the atom cannot be activated..
                if (get_solver().match(*atm, *a))
                    new_resolver<unify_atom>(*this, utils::s_ptr_cast<atom>(a));
            }

        if (atm->is_fact())
            if (get_resolvers().empty())
                new_resolver<activate_fact>(*this, get_phi());
            else
                new_resolver<activate_fact>(*this);
        else if (get_resolvers().empty())
            new_resolver<activate_goal>(*this, get_phi());
        else
            new_resolver<activate_goal>(*this);
    }

    json::json atom_flaw::to_json() const
    {
        auto j = stflaw::to_json();
        j["type"] = "atom";
        j["atom"] = {{"id", static_cast<uint64_t>(atm->get_id())}, {"is_fact", atm->is_fact()}, {"pred", atm->get_type().get_name().c_str()}, {"sigma", static_cast<uint64_t>(variable(atm->get_sigma()))}};
        return j;
    }

    activate_fact::activate_fact(atom_flaw &f) noexcept : stresolver(f, utils::rational(1)) {}
    activate_fact::activate_fact(atom_flaw &f, const utils::lit &rho) noexcept : stresolver(f, utils::rational(1), rho) {}

    void activate_fact::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False);                                                                      // The resolver cannot be negated..
        assert(get_solver().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::False); // The atom is not necessarily inactive..

        // activating this resolver activates the goal..
        get_solver().add_clause({!get_rho(), static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()});
    }

    json::json activate_fact::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "activate_fact";
        return j;
    }

    activate_goal::activate_goal(atom_flaw &f) noexcept : stresolver(f, utils::rational(1)) {}
    activate_goal::activate_goal(atom_flaw &f, const utils::lit &rho) noexcept : stresolver(f, utils::rational(1), rho) {}

    void activate_goal::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False);                                                                      // The resolver cannot be negated..
        assert(get_solver().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::False); // The atom is not necessarily inactive..

        // activating this resolver activates the goal..
        get_solver().add_clause({!get_rho(), static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()});

        // we call the corresponding rule..
        static_cast<riddle::predicate &>(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_type()).call(static_cast<atom_flaw &>(get_flaw()).get_atom());
    }

    json::json activate_goal::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "activate_goal";
        return j;
    }

    unify_atom::unify_atom(atom_flaw &f, atom_expr atm) noexcept : stresolver(f, utils::rational(1)), atm(atm) {}

    void unify_atom::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False);                                                                     // The resolver cannot be negated..
        assert(get_solver().value(static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()) != utils::True); // The atom must be unifiable
        assert(get_solver().value(atm->get_sigma()) != utils::False);                                            // The target atom must be activable..

        // we associate the unification constraints with the rho literal..
        get_solver().make_eq(static_cast<atom &>(*static_cast<atom_flaw &>(get_flaw()).get_atom()), *atm, get_rho());

        if (get_state() == utils::False)
            return; // The equality constraint cannot be satisfied..

        // we add a causal link from the target atom's flaw to this resolver..
        get_solver().add_causal_link(atm->get_flaw(), *this);

        // as a consequence of the activation of this resolver:
        //  - we make the current atom's sigma false (unified atom)..
        get_solver().add_clause({!get_rho(), !static_cast<atom_flaw &>(get_flaw()).get_atom()->get_sigma()});
        //  - and we make the target atom's sigma true (active atom)..
        get_solver().add_clause({!get_rho(), atm->get_sigma()});

        if (atm->get_flaw().get_state() != utils::True)
            get_solver().landmark_candidates.insert(&atm->get_flaw()); // we add the target atom's flaw to the set of landmarks..
    }

    json::json unify_atom::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "unify_atom";
        j["target"] = static_cast<uint64_t>(atm->get_id());
        return j;
    }

    mutex_flaw::mutex_flaw(resolver &n_r, resolver &c_r) noexcept : stflaw(static_cast<solver &>(n_r.get_flaw().get_graph()), std::vector<utils::ref_wrapper<resolver>>{c_r}), n_r(n_r), c_r(c_r) {}

    void mutex_flaw::compute_resolvers()
    {
        assert(get_solver().value(get_phi()) == get_state());
        for (const auto &r : n_r.get_flaw().get_resolvers())
            if (&*r == &n_r || r->get_state() == utils::False)
                continue; // we skip the mutex resolver..
            else
                new_resolver<mutex_resolver>(*this, static_cast<stresolver &>(*r));
    }

    json::json mutex_flaw::to_json() const
    {
        auto j = stflaw::to_json();
        j["type"] = "h2flaw";
        j["n_resolver"] = static_cast<uint64_t>(n_r.get_id());
        j["c_resolver"] = static_cast<uint64_t>(c_r.get_id());
        return j;
    }

    mutex_resolver::mutex_resolver(mutex_flaw &f, const stresolver &r) noexcept : stresolver(f, utils::rational(1), r.get_rho()), r(r) {}

    void mutex_resolver::apply()
    {
        assert(get_solver().value(get_rho()) == get_state());
        assert(get_state() != utils::False); // The resolver cannot be negated..
        for (const auto &pre : r.get_preconditions())
            get_solver().add_causal_link(*pre, *this);
    }

    json::json mutex_resolver::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "mutex_resolver";
        return j;
    }
} // namespace ratio
