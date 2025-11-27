#include "basic_flaws.hpp"
#include "basic_solver.hpp"
#include "conjunction.hpp"
#include "logging.hpp"
#include <stack>
#include <cassert>

namespace ratio
{

    flaw::flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) : slv(slv), causes(causes)
    {
        for (auto &cause : causes)
            cause.get().preconditions.push_back(*this); // this flaw is a precondition of its `cause` cause..
    }

    json::json flaw::to_json() const
    {
        json::json j_flaw{{"cost", linspire::to_json(est_cost)}, {"position", position}};
        if (!causes.empty())
        {
            json::json j_causes(json::json_type::array);
            for (const auto &c : causes)
                j_causes.push_back(c.get().get_id());
            j_flaw["causes"] = std::move(j_causes);
        }
        return j_flaw;
    }

    resolver::resolver(flaw &flw, utils::rational &&intrinsic_cost) : flw(flw), intrinsic_cost(intrinsic_cost) {}

    utils::rational resolver::resolver::get_estimated_cost() const noexcept
    {
#ifdef H_ADD
        // we compute the cost of the resolver as the sum of its intrinsic cost and the estimated costs of its preconditions..
        return std::accumulate(preconditions.begin(), preconditions.end(), intrinsic_cost, [](const auto &lhs, const auto &prec)
                               { return lhs + prec.get().get_estimated_cost(); });
#elif defined(H_MAX)
        // we compute the cost of the resolver as the sum of its intrinsic cost and the maximum of its preconditions' estimated costs..
        return intrinsic_cost + (*std::max_element(preconditions.begin(), preconditions.end(), [](const auto &lhs, const auto &rhs)
                                                   { return lhs.get().get_estimated_cost() < rhs.get().get_estimated_cost(); }))
                                    .get()
                                    .get_estimated_cost();
#else
        static_assert(false, "No heuristic defined for resolver cost estimation");
#endif
    }

    json::json resolver::to_json() const
    {
        json::json j_resolver{{"flaw", flw.get_id()}, {"intrinsic_cost", linspire::to_json(intrinsic_cost)}};
        if (!preconditions.empty())
        {
            json::json j_preconditions(json::json_type::array);
            for (const auto &p : preconditions)
                j_preconditions.push_back(p.get().get_id());
            j_resolver["preconditions"] = std::move(j_preconditions);
        }
        return j_resolver;
    }

    enum_flaw::enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : flaw(slv, std::move(causes)), var(std::make_shared<enum_item>(tp, std::move(values), ev, *this)) {}

    void enum_flaw::compute_resolvers()
    {
        if (expanded)
            return;
        // Create a resolver for each possible value..
        auto &dom = get_ac().domain(utils::variable(static_cast<const riddle::enum_item &>(*var).get_var()));
        for (auto &val : var->get_values())
            if (dom.count(&static_cast<const utils::enum_val &>(*val)))
                new_resolver<choose_val>(*this, val);
        expanded = true;
    }

    choose_val::choose_val(enum_flaw &flw, riddle::expr val) noexcept : resolver(flw, utils::rational(1)), val(std::move(val)) {}
    bool choose_val::apply() noexcept
    {
        auto &e_item = static_cast<riddle::enum_item &>(*static_cast<enum_flaw &>(flw).get_var());
        ctx.ac_cnsts.push_back(get_ac_solver().new_assign(utils::variable(e_item.get_var()), static_cast<utils::enum_val &>(*val)));
        return true;
    }

    clause_flaw::clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause) noexcept : flaw(slv, std::move(causes)), clause(std::move(clause)) {}

    void clause_flaw::compute_resolvers()
    { // Create a resolver for each literal in the clause..
        for (const auto &lit : clause)
        {
            auto &c_lit = static_cast<const riddle::bool_item &>(*lit).get_lit();
            if (get_ac().allows(utils::variable(c_lit), utils::sign(c_lit) ? arc_consistency::solver::True : arc_consistency::solver::False))
                new_resolver<choose_lit>(*this, lit);
        }
    }

    json::json clause_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "clause";
        return j;
    }

    choose_lit::choose_lit(clause_flaw &flw, riddle::bool_expr lit) noexcept : resolver(flw, utils::rational(1)), lit(lit) {}
    bool choose_lit::apply() noexcept
    {
        auto &c_lit = static_cast<const riddle::bool_item &>(*lit).get_lit();
        ctx.ac_cnsts.push_back(get_ac_solver().new_assign(utils::variable(c_lit), utils::sign(c_lit) ? arc_consistency::solver::True : arc_consistency::solver::False));
        return true;
    }

    json::json choose_lit::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "lit";
        j["data"]["lit"] = to_string(static_cast<const riddle::bool_item &>(*lit).get_lit());
        return j;
    }

    disjunction_flaw::disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept : flaw(slv, std::move(causes)), disjuncts(std::move(disjuncts)) {}

    void disjunction_flaw::compute_resolvers()
    { // Create a resolver for each disjunct..
        for (const auto &disjunct : disjuncts)
            new_resolver<choose_conjunction>(*this, *disjunct);
    }

    json::json disjunction_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "disjunction";
        return j;
    }

    choose_conjunction::choose_conjunction(disjunction_flaw &flw, riddle::conjunction &conj) noexcept : resolver(flw, utils::rational(1)), conj(conj) {}
    bool choose_conjunction::apply() noexcept
    {
        conj.execute();
        return true;
    }

    json::json choose_conjunction::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "disjunct";
        return j;
    }

    atom_flaw::atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : flaw(slv, std::move(causes)), atm(std::make_shared<atom>(pred, is_fact, std::move(args), std::move(sigma), *this)) {}

    void atom_flaw::compute_resolvers()
    { // Create a unify resolver for each inactive ancestor atom..
        assert(atm->get_state() == riddle::atom_state::inactive);
        for (auto &a : static_cast<riddle::predicate &>(atm->get_type()).get_atoms())
            if (a->get_state() == riddle::active && !have_common_ancestors(a, atm) && slv.match(*atm, *a))
                new_resolver<unify_atom>(*this, a);

        // Create an activate resolver..
        if (atm->is_fact())
            new_resolver<activate_fact>(*this);
        else
            new_resolver<activate_goal>(*this);
    }

    bool atom_flaw::have_common_ancestors(const riddle::atom_expr &ancestor, const riddle::atom_expr &descendant)
    {
        flaw *curr_f = &static_cast<atom &>(*descendant).get_flaw();
        std::unordered_set<flaw *> visited;
        while (curr_f)
        {
            visited.insert(curr_f);
            if (curr_f == &static_cast<atom &>(*ancestor).get_flaw())
                return true;
            curr_f = curr_f->get_causes().empty() ? nullptr : &curr_f->get_causes().front().get().get_flaw();
        }

        flaw *anc_f = &static_cast<atom &>(*ancestor).get_flaw();
        while (anc_f)
        {
            if (visited.count(anc_f))
                return true;
            anc_f = anc_f->get_causes().empty() ? nullptr : &anc_f->get_causes().front().get().get_flaw();
        }
        return false;
    }

    json::json atom_flaw::to_json() const
    {
        auto j = flaw::to_json();
        j["data"]["type"] = "atom";
        j["data"]["atom"] = {{"id", atm->get_id()}, {"is_fact", atm->is_fact()}, {"predicate", atm->get_type().get_name()}};
        return j;
    }

    activate_fact::activate_fact(atom_flaw &flw) noexcept : resolver(flw, utils::rational(1)) {}
    bool activate_fact::apply() noexcept
    {
        ctx.ac_cnsts.push_back(get_ac_solver().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::True));
        return true;
    }

    json::json activate_fact::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_fact";
        return j;
    }

    activate_goal::activate_goal(atom_flaw &flw) noexcept : resolver(flw, utils::rational(1)) {}
    bool activate_goal::apply() noexcept
    {
        ctx.ac_cnsts.push_back(get_ac_solver().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::True));
        try
        {
            static_cast<riddle::predicate &>(static_cast<atom_flaw &>(flw).get_atom()->get_type()).call(static_cast<atom_flaw &>(flw).get_atom());
            return true;
        }
        catch (const std::exception &e)
        {
            return false;
        }
    }

    json::json activate_goal::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "activate_goal";
        return j;
    }

    unify_atom::unify_atom(atom_flaw &flw, riddle::atom_expr atm) noexcept : resolver(flw, utils::rational(2)), atm(std::move(atm)) {}
    bool unify_atom::apply() noexcept
    {
        ctx.ac_cnsts.push_back(get_ac_solver().new_assign(utils::variable(static_cast<riddle::atom &>(*static_cast<atom_flaw &>(flw).get_atom()).get_sigma()), arc_consistency::solver::False));
        return execute(get_solver().new_eq(static_cast<atom_flaw &>(flw).get_atom(), atm));
    }

    json::json unify_atom::to_json() const
    {
        auto j = resolver::to_json();
        j["data"]["type"] = "unify_atom";
        j["data"]["atom_id"] = atm->get_id();
        return j;
    }
} // namespace ratio
