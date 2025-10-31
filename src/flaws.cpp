#include "flaws.hpp"
#include "solver.hpp"

namespace ratio
{
    flaw::flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive) noexcept : slv(slv), causes(std::move(causes)), exclusive(exclusive)
    {
        if (causes.empty())
        { // if there are no causes, the flaw is a root flaw, so it is active by default..
            state = utils::True;
            slv.active_flaws.insert(this);
        }
    }

    json::json flaw::to_json() const
    {
        json::json j_flaw{{"cost", linspire::to_json(est_cost)}, {"state", to_string(state)}};
        if (!causes.empty())
        {
            json::json j_causes(json::json_type::array);
            for (const auto &c : causes)
                j_causes.push_back(c.get().get_id());
            j_flaw["causes"] = std::move(j_causes);
        }
        return j_flaw;
    }

    resolver::resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept : f(f), intrinsic_cost(std::move(intrinsic_cost)), cnst(std::make_shared<linspire::constraint>()) {}

    json::json resolver::to_json() const
    {
        json::json j_resolver{{"flaw", f.get_id()}, {"intrinsic_cost", linspire::to_json(intrinsic_cost)}, {"state", to_string(state)}};
        return j_resolver;
    }

    enum_flaw::enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept : flaw(slv, std::move(causes)), var(std::move(var)) {}

    void enum_flaw::compute_resolvers() {}

    clause_flaw::clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive) noexcept : flaw(slv, std::move(causes), exclusive), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers() {}

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers() {}

    atom_flaw::atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(*this, pred, is_fact, std::move(args), std::move(sigma))) {}

    void atom_flaw::compute_resolvers() {}

    json::json atom_flaw::to_json() const
    {
        json::json j_flaw = flaw::to_json();
        j_flaw["data"] = {{"type", "atom"}, {"atom", {{"atom_id", atm->get_id()}, {"is_fact", atm->is_fact()}, {"predicate", atm->get_type().get_name()}, {"sigma", 0}}}};
        return j_flaw;
    }
} // namespace ratio
