#include "stflaws.hpp"
#include "conjunction.hpp"
#include <cassert>

namespace ratio
{
    stflaw::stflaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), phi(compute_phi(slv, get_causes())), pos(slv.net.new_tp())
    {
        for (const auto &cause : causes) // we impose the position constraint (i.e., the flaw must be before its causes) to avoid causality loops..
            slv.net.new_distance(static_cast<stflaw &>(cause->get_flaw()).get_pos(), pos, -utils::rational::one);
    }

    utils::lit stflaw::compute_phi(stsolver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept
    {
        auto phi = utils::lit(slv.net.new_var());
        std::vector<utils::lit> ls;
        for (const auto &cause : causes)
            ls.push_back(!static_cast<stresolver &>(*cause).get_rho());
        ls.push_back(phi);
        slv.net.new_clause(std::move(ls));
        return phi;
    }

    void stflaw::expanded_flaw()
    {
        std::vector<utils::lit> ls;
        for (const auto &resolver : get_resolvers())
            ls.push_back(static_cast<stresolver &>(*resolver).get_rho());
        if (static_cast<stsolver &>(get_graph()).net.value(phi) == utils::True)
        {
            if (is_exclusive())
                for (size_t i = 0; i < ls.size(); ++i)
                    for (size_t j = i + 1; j < ls.size(); ++j)
                        static_cast<stsolver &>(get_graph()).net.new_clause({!ls[i], !ls[j]});
            static_cast<stsolver &>(get_graph()).net.new_clause(std::move(ls));
        }
        else
        {
            if (is_exclusive())
                for (size_t i = 0; i < ls.size(); ++i)
                    for (size_t j = i + 1; j < ls.size(); ++j)
                        static_cast<stsolver &>(get_graph()).net.new_clause({!phi, !ls[i], !ls[j]});
            ls.push_back(!phi);
            static_cast<stsolver &>(get_graph()).net.new_clause(std::move(ls));
        }
    }

    json::json stflaw::to_json() const
    {
        json::json j = flaw::to_json();
        j["phi"] = to_string(phi).c_str();
        j["pos"] = pos;
        return j;
    }

    stresolver::stresolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : stresolver(f, std::move(intrinsic_cost), utils::lit(static_cast<stsolver &>(f.get_graph()).net.new_var())) {}
    stresolver::stresolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept : resolver(f, std::move(intrinsic_cost)), rho(rho) { static_cast<stsolver &>(f.get_graph()).net.new_clause({!rho, static_cast<stflaw &>(f).get_phi()}); }

    [[nodiscard]] json::json stresolver::to_json() const
    {
        json::json j = resolver::to_json();
        j["rho"] = to_string(rho).c_str();
        return j;
    }

    stenum_flaw::stenum_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) noexcept : stflaw(slv, std::move(causes), true), var(create_var(tp, std::move(values))) {}
    utils::s_ptr<riddle::enum_item> stenum_flaw::create_var(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values)
    {
        assert(!values.empty());
        std::vector<utils::lit> lits;
        if (values.size() == 1)
            lits.push_back(utils::TRUE_lit);
        else
            for (size_t i = 0; i < values.size(); i++)
                lits.push_back(utils::lit(static_cast<stsolver &>(tp.get_scope().get_core()).net.new_var()));
        return utils::make_s_ptr<riddle::enum_item>(tp, std::move(values), std::move(lits));
    }
    void stenum_flaw::compute_resolvers()
    {
        for (const auto &val : var->get_values())
            new_resolver<stchoose_val>(*this, *val);
    }

    stchoose_val::stchoose_val(stenum_flaw &f, const utils::enum_val &val) noexcept : stresolver(f, utils::rational(1), f.get_var()->get_lit(val)), val(val) {}
    void stchoose_val::apply() {}

    stclause_flaw::stclause_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause, const bool &exclusive) noexcept : stflaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}
    void stclause_flaw::compute_resolvers()
    {
        for (const auto &lit : clause)
            new_resolver<stchoose_lit>(*this, lit);
    }

    stchoose_lit::stchoose_lit(stclause_flaw &f, const utils::lit &conj) noexcept : stresolver(f, utils::rational(1), conj) {}
    void stchoose_lit::apply() {}

    stdisjunction_flaw::stdisjunction_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept : stflaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}
    void stdisjunction_flaw::compute_resolvers()
    {
        for (const auto &disjunct : disjuncts)
            new_resolver<stchoose_conjunction>(*this, *disjunct);
    }

    stchoose_conjunction::stchoose_conjunction(stdisjunction_flaw &f, riddle::conjunction &conj) noexcept : stresolver(f, utils::rational(1)), conj(conj) {}
    void stchoose_conjunction::apply() { conj.execute(); }

    statom_flaw::statom_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) noexcept : stflaw(slv, std::move(causes)), atm(utils::make_s_ptr<atom>(*this, pred, is_fact, std::move(args), utils::lit(slv.net.new_var()))) {}
    void statom_flaw::compute_resolvers() {}

    json::json statom_flaw::to_json() const
    {
        auto j = stflaw::to_json();
        j["type"] = "atom";
        j["atom"] = {{"id", static_cast<uint64_t>(atm->get_id())}, {"is_fact", atm->is_fact()}, {"pred", atm->get_type().get_name().c_str()}, {"sigma", static_cast<uint64_t>(variable(atm->get_sigma()))}};
        return j;
    }

    stactivate_fact::stactivate_fact(statom_flaw &f) noexcept : stresolver(f, utils::rational(1)) {}
    stactivate_fact::stactivate_fact(statom_flaw &f, const utils::lit &rho) noexcept : stresolver(f, utils::rational(1), rho) {}

    void stactivate_fact::apply() {}

    json::json stactivate_fact::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "activate_fact";
        return j;
    }

    stactivate_goal::stactivate_goal(statom_flaw &f) noexcept : stresolver(f, utils::rational(1)) {}
    stactivate_goal::stactivate_goal(statom_flaw &f, const utils::lit &rho) noexcept : stresolver(f, utils::rational(1), rho) {}

    void stactivate_goal::apply() {}

    json::json stactivate_goal::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "activate_goal";
        return j;
    }

    stunify_atom::stunify_atom(statom_flaw &f, atom_expr atm) noexcept : stresolver(f, utils::rational(1)), atm(atm) {}

    void stunify_atom::apply() {}

    json::json stunify_atom::to_json() const
    {
        auto j = stresolver::to_json();
        j["type"] = "unify_atom";
        j["target"] = static_cast<uint64_t>(atm->get_id());
        return j;
    }
} // namespace ratio
