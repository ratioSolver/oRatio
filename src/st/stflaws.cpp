#include "stflaws.hpp"
#include "conjunction.hpp"

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

    stresolver::stresolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : stresolver(f, std::move(intrinsic_cost), utils::lit(static_cast<stsolver &>(f.get_graph()).net.new_var())) {}
    stresolver::stresolver(flaw &f, utils::rational &&intrinsic_cost, const utils::lit &rho) noexcept : resolver(f, std::move(intrinsic_cost)), rho(rho) { static_cast<stsolver &>(f.get_graph()).net.new_clause({!rho, static_cast<stflaw &>(f).get_phi()}); }

    stclause_flaw::stclause_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::lit> &&clause, const bool &exclusive) noexcept : stflaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}
    void stclause_flaw::compute_resolvers()
    {
        for (const auto &lit : clause)
            new_resolver<stchoose_lit>(*this, lit);
    }

    stchoose_lit::stchoose_lit(stclause_flaw &f, const utils::lit &conj) noexcept : stresolver(f, utils::rational(1)), conj(conj) {}
    void stchoose_lit::apply() {}

    stdisjunction_flaw::stdisjunction_flaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes, std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) noexcept : stflaw(slv, std::move(causes), false), disjuncts(std::move(disjuncts)) {}
    void stdisjunction_flaw::compute_resolvers()
    {
        for (const auto &disjunct : disjuncts)
            new_resolver<stchoose_conjunction>(*this, *disjunct);
    }

    stchoose_conjunction::stchoose_conjunction(stdisjunction_flaw &f, riddle::conjunction &conj) noexcept : stresolver(f, utils::rational(1)), conj(conj) {}
    void stchoose_conjunction::apply() { conj.execute(); }
} // namespace ratio
