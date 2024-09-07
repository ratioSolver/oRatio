#include <cassert>
#include <algorithm>
#include "graph.hpp"
#include "init.hpp"
#include "smart_type.hpp"
#include "atom_flaw.hpp"
#include "bool_flaw.hpp"
#include "disj_flaw.hpp"
#include "disjunction_flaw.hpp"
#include "agent.hpp"
#include "state_variable.hpp"
#include "reusable_resource.hpp"
#include "consumable_resource.hpp"
#include "logging.hpp"

#ifdef ENABLE_API
#define STATE_CHANGED() state_changed()
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
    atom::atom(riddle::predicate &p, bool is_fact, atom_flaw &reason, std::map<std::string, std::shared_ptr<item>, std::less<>> &&args) : riddle::atom(p, is_fact, utils::lit(static_cast<solver &>(p.get_scope().get_core()).get_sat().new_var()), std::move(args)), reason(reason) {}

    solver::solver(std::string_view name) noexcept : name(name), sat(), lra(sat.new_theory<semitone::lra_theory>()), idl(sat.new_theory<semitone::idl_theory>()), rdl(sat.new_theory<semitone::rdl_theory>()), ov(sat.new_theory<semitone::ov_theory>()), gr(sat.new_theory<graph>(*this)) {}

    void solver::init()
    {
        LOG_DEBUG("[" << name << "] Initializing solver");
        // we read the init string..
        core::read(INIT_STRING);

        add_type(std::make_unique<agent>(*this));
        add_type(std::make_unique<state_variable>(*this));
        add_type(std::make_unique<reusable_resource>(*this));
        add_type(std::make_unique<consumable_resource>(*this));

        if (!sat.propagate())
            throw riddle::unsolvable_exception();
        STATE_CHANGED();
    }

    void solver::read(const std::string &script)
    {
        LOG_DEBUG("[" << name << "] Reading script: " << script);
        core::read(script);

        if (!sat.propagate()) // we propagate the constraints..
            throw riddle::unsolvable_exception();
        reset_smart_types(); // we reset the smart types..

        STATE_CHANGED();
    }

    void solver::read(const std::vector<std::string> &files)
    {
        LOG_DEBUG("[" << name << "] Reading problem files");
        core::read(files);

        if (!sat.propagate()) // we propagate the constraints..
            throw riddle::unsolvable_exception();
        reset_smart_types(); // we reset the smart types..

        STATE_CHANGED();
    }

    bool solver::solve()
    {
        LOG_DEBUG("[" << name << "] Solving problem");
        gr.build();
#ifdef CHECK_INCONSISTENCIES
        // we solve all the current inconsistencies..
        solve_inconsistencies();

        while (gr.has_active_flaws())
        {
            while (gr.has_infinite_cost_active_flaws())
                next(); // we move to the next state..

            auto &f = gr.get_most_expensive_flaw(); // we get the most expensive flaw..
            CURRENT_FLAW(f);
            auto &r = f.get_best_resolver(); // we get the best resolver for the flaw..
            CURRENT_RESOLVER(r);

            // we take the decision by activating the best resolver..
            take_decision(r.get_rho());

            // we solve all the current inconsistencies..
            solve_inconsistencies();
        }
#else
        do
        {
            while (gr.has_active_flaws())
            {
                while (gr.has_infinite_cost_active_flaws())
                    next(); // we move to the next state..

                auto &f = gr.get_most_expensive_flaw(); // we get the most expensive flaw..
                CURRENT_FLAW(f);
                auto &r = f.get_best_resolver(); // we get the best resolver for the flaw..
                CURRENT_RESOLVER(r);

                // we take the decision by activating the best resolver..
                take_decision(r.get_rho());
            }
            // we solve all the current inconsistencies..
            solve_inconsistencies();
        } while (gr.has_active_flaws());
#endif
        LOG_DEBUG("[" << name << "] Problem solved");
        return true;
    }

    void solver::take_decision(const utils::lit &d)
    {
        assert(sat.value(d) == utils::Undefined);

        LOG_DEBUG("[" << name << "] Taking decision: " << to_string(d));
        // we take the decision..
        if (!sat.assume(d))
            throw riddle::unsolvable_exception();
        STATE_CHANGED();
    }

    void solver::next()
    {
        LOG_DEBUG("[" << name << "] Next..");
        if (!sat.next())
            throw riddle::unsolvable_exception();

        if (sat.root_level())
            gr.build(); // we make sure the graph is built..

        STATE_CHANGED();
    }

    std::shared_ptr<riddle::bool_item> solver::new_bool() noexcept
    {
        auto b = std::make_shared<riddle::bool_item>(get_bool_type(), utils::lit(sat.new_var()));
        if (sat.value(b->get_value()) == utils::Undefined)
            gr.new_flaw<bool_flaw>(*this, std::vector<std::reference_wrapper<resolver>>(), b->get_value());
        return b;
    }
    std::shared_ptr<riddle::arith_item> solver::new_int() noexcept { return std::make_shared<riddle::arith_item>(get_int_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::arith_item> solver::new_real() noexcept { return std::make_shared<riddle::arith_item>(get_real_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::arith_item> solver::new_time() noexcept { return std::make_shared<riddle::arith_item>(get_time_type(), utils::lin(rdl.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::enum_item> solver::new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) { return std::make_shared<riddle::enum_item>(tp, ov.new_var(std::move(values), false)); }

    std::shared_ptr<riddle::arith_item> solver::minus(const std::shared_ptr<riddle::arith_item> &xpr)
    {
        assert(is_arith(*xpr));
        if (is_int(*xpr))
            return std::make_shared<riddle::arith_item>(get_int_type(), -xpr->get_value());
        else if (is_time(*xpr))
            return std::make_shared<riddle::arith_item>(get_time_type(), -xpr->get_value());
        else
        {
            assert(is_real(*xpr));
            return std::make_shared<riddle::arith_item>(get_real_type(), -xpr->get_value());
        }
    }

    std::shared_ptr<riddle::arith_item> solver::add(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin sum;
        for (const auto &xpr : xprs)
            sum = sum + xpr->get_value();
        auto &tp = determine_type(xprs);
        if (&get_int_type() == &tp)
            return std::make_shared<riddle::arith_item>(get_int_type(), sum);
        assert(&get_real_type() == &tp);
        return std::make_shared<riddle::arith_item>(get_real_type(), sum);
    }

    std::shared_ptr<riddle::arith_item> solver::sub(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin diff = xprs[0]->get_value();
        for (size_t i = 1; i < xprs.size(); i++)
            diff -= xprs[i]->get_value();
        auto &tp = determine_type(xprs);
        if (&get_int_type() == &tp)
            return std::make_shared<riddle::arith_item>(get_int_type(), diff);
        else if (&get_real_type() == &tp)
            return std::make_shared<riddle::arith_item>(get_real_type(), diff);
        assert(&get_time_type() == &tp);
        return std::make_shared<riddle::arith_item>(get_time_type(), diff);
    }

    std::shared_ptr<riddle::arith_item> solver::mul(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs)
    {
        assert(xprs.size() > 1);
        if (auto var_it = std::find_if(xprs.begin(), xprs.end(), [this](const auto &x)
                                       { return !is_constant(*x); });
            var_it != xprs.end())
        {
            utils::lin prod = (*var_it)->get_value();
            for (const auto &xpr : xprs)
                if (xpr != *var_it)
                {
                    assert(is_arith(*xpr));
                    assert(is_constant(*xpr) && arithmetic_value(*xpr).get_infinitesimal() == 0);
                    prod = prod * arithmetic_value(*xpr).get_rational();
                }
            auto &tp = determine_type(xprs);
            if (&get_int_type() == &tp)
                return std::make_shared<riddle::arith_item>(get_int_type(), prod);
            assert(&get_real_type() == &tp);
            return std::make_shared<riddle::arith_item>(get_real_type(), prod);
        }
        else
        {
            assert(std::all_of(xprs.begin(), xprs.end(), [this](const auto &x)
                               { return is_arith(*x) && is_constant(*x) && is_zero(arithmetic_value(*x).get_infinitesimal()); }));
            utils::rational prod = xprs[0]->get_type().get_scope().get_core().arithmetic_value(*xprs[0]).get_rational();
            for (size_t i = 1; i < xprs.size(); i++)
                prod = prod * xprs[i]->get_type().get_scope().get_core().arithmetic_value(*xprs[i]).get_rational();
            auto &tp = determine_type(xprs);
            if (&get_int_type() == &tp)
                return std::make_shared<riddle::arith_item>(get_int_type(), utils::lin(prod));
            assert(&get_real_type() == &tp);
            return std::make_shared<riddle::arith_item>(get_real_type(), utils::lin(prod));
        }
    }

    std::shared_ptr<riddle::arith_item> solver::div(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs)
    {
        assert(xprs.size() > 1);
        utils::lin quot = xprs[0]->get_value();
        for (size_t i = 1; i < xprs.size(); i++)
        {
            assert(is_arith(*xprs[i]));
            assert(is_constant(*xprs[i]) && is_zero(arithmetic_value(*xprs[i]).get_infinitesimal()));
            quot = quot / arithmetic_value(*xprs[i]).get_rational();
        }
        auto &tp = determine_type(xprs);
        if (&get_int_type() == &tp)
            return std::make_shared<riddle::arith_item>(get_int_type(), quot);
        assert(&get_real_type() == &tp);
        return std::make_shared<riddle::arith_item>(get_real_type(), quot);
    }

    std::shared_ptr<riddle::bool_item> solver::lt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) { return std::make_shared<riddle::bool_item>(get_bool_type(), lra.new_lt(lhs->get_value(), rhs->get_value())); }
    std::shared_ptr<riddle::bool_item> solver::leq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) { return std::make_shared<riddle::bool_item>(get_bool_type(), lra.new_leq(lhs->get_value(), rhs->get_value())); }
    std::shared_ptr<riddle::bool_item> solver::gt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) { return std::make_shared<riddle::bool_item>(get_bool_type(), lra.new_lt(rhs->get_value(), lhs->get_value())); }
    std::shared_ptr<riddle::bool_item> solver::geq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) { return std::make_shared<riddle::bool_item>(get_bool_type(), lra.new_leq(rhs->get_value(), lhs->get_value())); }

    std::shared_ptr<riddle::bool_item> solver::eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs)
    {
        if (&*lhs == &*rhs) // the two items are the same item..
            return core::new_bool(true);
        else if (is_bool(*lhs) && is_bool(*rhs))
            return std::make_shared<riddle::bool_item>(get_bool_type(), sat.new_eq(std::static_pointer_cast<riddle::bool_item>(lhs)->get_value(), std::static_pointer_cast<riddle::bool_item>(rhs)->get_value()));
        else if (is_arith(*lhs) && is_arith(*rhs))
        {
            if ((is_int(*lhs) || is_real(*lhs)) && (is_int(*rhs) || is_real(*rhs)))
                return std::make_shared<riddle::bool_item>(get_bool_type(), lra.new_eq(std::static_pointer_cast<riddle::arith_item>(lhs)->get_value(), std::static_pointer_cast<riddle::arith_item>(rhs)->get_value()));
            else
            {
                assert(is_time(*lhs) && is_time(*rhs));
                return std::make_shared<riddle::bool_item>(get_bool_type(), rdl.new_eq(std::static_pointer_cast<riddle::arith_item>(lhs)->get_value(), std::static_pointer_cast<riddle::arith_item>(rhs)->get_value()));
            }
        }
        else if (is_string(*lhs) && is_string(*rhs))
            return std::make_shared<riddle::bool_item>(get_bool_type(), std::static_pointer_cast<riddle::string_item>(lhs)->get_value() == std::static_pointer_cast<riddle::string_item>(rhs)->get_value() ? utils::TRUE_lit : utils::FALSE_lit);
        else if (is_enum(*lhs))
        {
            if (is_enum(*rhs))
                return std::make_shared<riddle::bool_item>(get_bool_type(), ov.new_eq(std::static_pointer_cast<riddle::enum_item>(lhs)->get_value(), std::static_pointer_cast<riddle::enum_item>(rhs)->get_value()));
            else
            {
                assert(is_constant(*rhs));
                return std::make_shared<riddle::bool_item>(get_bool_type(), ov.allows(std::static_pointer_cast<riddle::enum_item>(lhs)->get_value(), static_cast<utils::enum_val &>(*rhs)));
            }
        }
        else if (is_enum(*rhs))
            return eq(rhs, lhs);
        else if (&lhs->get_type() == &rhs->get_type())
        { // we are comparing two items of the same type..
            std::vector<utils::lit> eqs;
            if (auto p = dynamic_cast<riddle::predicate *>(&lhs->get_type()))
            { // we are comparing two atoms..
                auto l = static_cast<riddle::atom *>(lhs.operator->());
                auto r = static_cast<riddle::atom *>(rhs.operator->());
                std::queue<riddle::predicate *> q;
                q.push(p);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                        {
                            auto c_eq = eq(l->get(f_name), r->get(f_name));
                            if (bool_value(*c_eq) == utils::False)
                                return std::make_shared<riddle::bool_item>(get_bool_type(), utils::FALSE_lit);
                            eqs.push_back(c_eq->get_value());
                        }
                    for (const auto &pp : q.front()->get_parents())
                        q.push(&pp.get());
                    q.pop();
                }
            }
            else if (auto t = dynamic_cast<riddle::component_type *>(&lhs->get_type()))
            { // we are comparing two components..
                auto l = static_cast<riddle::component *>(lhs.operator->());
                auto r = static_cast<riddle::component *>(rhs.operator->());
                std::queue<riddle::component_type *> q;
                q.push(t);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic())
                        {
                            auto c_eq = eq(l->get(f_name), r->get(f_name));
                            if (bool_value(*c_eq) == utils::False)
                                return std::make_shared<riddle::bool_item>(get_bool_type(), utils::FALSE_lit);
                            eqs.push_back(c_eq->get_value());
                        }
                    for (const auto &pp : q.front()->get_parents())
                        q.push(&pp.get());
                    q.pop();
                }
            }
            else
                throw std::logic_error("Invalid comparison");
            switch (eqs.size())
            {
            case 0:
                return core::new_bool(false);
            case 1:
                return std::make_shared<riddle::bool_item>(get_bool_type(), eqs[0]);
            default:
                return std::make_shared<riddle::bool_item>(get_bool_type(), sat.new_conj(std::move(eqs)));
            }
        }
        else // the two items are different..
            return core::new_bool(false);
    }

    bool solver::matches(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) const noexcept
    {
        if (&*lhs == &*rhs) // the two items are the same item..
            return true;
        else if (is_bool(*lhs) && is_bool(*rhs)) // we are comparing two boolean items..
        {
            auto l = bool_value(*std::static_pointer_cast<riddle::bool_item>(lhs));
            auto r = bool_value(*std::static_pointer_cast<riddle::bool_item>(rhs));
            return l == r || l == utils::Undefined || r == utils::Undefined;
        }
        else if (is_arith(*lhs) && is_arith(*rhs)) // we are comparing two arithmetic items..
        {
            if ((is_int(*lhs) || is_real(*lhs)) && (is_int(*rhs) || is_real(*rhs))) // we are comparing two integer or real items..
                return lra.matches(std::static_pointer_cast<riddle::arith_item>(lhs)->get_value(), std::static_pointer_cast<riddle::arith_item>(rhs)->get_value());
            else // we are comparing two time items..
                return rdl.matches(std::static_pointer_cast<riddle::arith_item>(lhs)->get_value(), std::static_pointer_cast<riddle::arith_item>(rhs)->get_value());
        }
        else if (is_string(*lhs) && is_string(*rhs)) // we are comparing two string items..
            return std::static_pointer_cast<riddle::string_item>(lhs)->get_value() == std::static_pointer_cast<riddle::string_item>(rhs)->get_value();
        else if (is_enum(*lhs) && is_enum(*rhs)) // we are comparing two enum items..
            return ov.matches(std::static_pointer_cast<riddle::enum_item>(lhs)->get_value(), std::static_pointer_cast<riddle::enum_item>(rhs)->get_value());
        else if (&lhs->get_type() == &rhs->get_type()) // we are comparing two items of the same type..
        {
            if (auto p = dynamic_cast<riddle::predicate *>(&lhs->get_type()))
            { // we are comparing two atoms..
                auto l = static_cast<riddle::atom *>(lhs.operator->());
                auto r = static_cast<riddle::atom *>(rhs.operator->());
                std::queue<riddle::predicate *> q;
                q.push(p);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic() && !matches(l->get(f_name), r->get(f_name)))
                            return false;
                    for (const auto &pp : q.front()->get_parents())
                        q.push(&pp.get());
                    q.pop();
                }
            }
            else if (auto t = dynamic_cast<riddle::component_type *>(&lhs->get_type()))
            { // we are comparing two components..
                auto l = static_cast<riddle::component *>(lhs.operator->());
                auto r = static_cast<riddle::component *>(rhs.operator->());
                std::queue<riddle::component_type *> q;
                q.push(t);
                while (!q.empty())
                {
                    for (const auto &[f_name, f] : q.front()->get_fields())
                        if (!f->is_synthetic() && !matches(l->get(f_name), r->get(f_name)))
                            return false;
                    for (const auto &pp : q.front()->get_parents())
                        q.push(&pp.get());
                    q.pop();
                }
            }
            else
                return false; // invalid comparison..
            return true;
        }
        else // the two items are different..
            return false;
    }

    std::shared_ptr<riddle::bool_item> solver::conj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(xpr->get_value());
        return std::make_shared<riddle::bool_item>(get_bool_type(), sat.new_conj(std::move(lits)));
    }
    std::shared_ptr<riddle::bool_item> solver::disj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(xpr->get_value());
        auto causes = gr.get_current_resolver() ? std::vector<std::reference_wrapper<resolver>>{gr.get_current_resolver().value().get()} : std::vector<std::reference_wrapper<resolver>>();
        return std::make_shared<riddle::bool_item>(get_bool_type(), gr.new_flaw<disj_flaw>(*this, std::move(causes), std::move(lits)).get_phi());
    }
    std::shared_ptr<riddle::bool_item> solver::exct_one(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(xpr->get_value());
        auto causes = gr.get_current_resolver() ? std::vector<std::reference_wrapper<resolver>>{gr.get_current_resolver().value().get()} : std::vector<std::reference_wrapper<resolver>>();
        return std::make_shared<riddle::bool_item>(get_bool_type(), gr.new_flaw<disj_flaw>(*this, std::move(causes), std::move(lits), true).get_phi());
    }
    std::shared_ptr<riddle::bool_item> solver::negate(const std::shared_ptr<riddle::bool_item> &expr) { return std::make_shared<riddle::bool_item>(get_bool_type(), !expr->get_value()); }

    void solver::assert_fact(const std::shared_ptr<riddle::bool_item> &fact)
    {
        if (!sat.new_clause({!gr.get_ni(), fact->get_value()}))
            throw riddle::unsolvable_exception();
    }

    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept
    {
        auto causes = gr.get_current_resolver() ? std::vector<std::reference_wrapper<resolver>>{gr.get_current_resolver().value().get()} : std::vector<std::reference_wrapper<resolver>>();
        gr.new_flaw<disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }

    std::shared_ptr<riddle::atom> solver::new_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&arguments) noexcept
    {
        LOG_TRACE("Creating new " << pred.get_name() << " atom");
        auto causes = gr.get_current_resolver() ? std::vector<std::reference_wrapper<resolver>>{gr.get_current_resolver().value().get()} : std::vector<std::reference_wrapper<resolver>>();
        auto atm = gr.new_flaw<atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(arguments)).get_atom();
        // we check if we need to notify any smart types of the new goal..
        if (!is_core(atm->get_type().get_scope()))
        {
            std::queue<riddle::component_type *> q;
            q.push(static_cast<riddle::component_type *>(&atm->get_type().get_scope())); // we start from the scope of the atom..
            while (!q.empty())
            {
                if (auto st = dynamic_cast<smart_type *>(q.front()))
                    st->new_atom(atm);
                for (const auto &p : q.front()->get_parents())
                    q.push(&p.get());
                q.pop();
            }
        }
        return atm;
    }

    utils::lbool solver::bool_value(const riddle::bool_item &expr) const noexcept { return sat.value(expr.get_value()); }
    utils::inf_rational solver::arithmetic_value(const riddle::arith_item &expr) const noexcept
    {
        assert(is_arith(expr));
        if (is_int(expr) || is_real(expr))
            return lra.value(expr.get_value());
        else
        {
            assert(is_time(expr));
            return rdl.bounds(expr.get_value()).first;
        }
    }
    std::pair<utils::inf_rational, utils::inf_rational> solver::bounds(const riddle::arith_item &expr) const noexcept
    {
        assert(is_arith(expr));
        if (is_int(expr) || is_real(expr))
            return lra.bounds(expr.get_value());
        else
        {
            assert(is_time(expr));
            return rdl.bounds(expr.get_value());
        }
    }
    std::vector<std::reference_wrapper<utils::enum_val>> solver::domain(const riddle::enum_item &expr) const noexcept
    {
        assert(is_enum(expr));
        return ov.domain(expr.get_value());
    }
    bool solver::assign(const riddle::enum_item &expr, utils::enum_val &val)
    {
        assert(is_enum(expr));
        return ov.assign(expr.get_value(), val);
    }
    void solver::forbid(const riddle::enum_item &expr, utils::enum_val &val)
    {
        assert(is_enum(expr));
        if (!ov.forbid(expr.get_value(), val))
            throw riddle::unsolvable_exception();
    }

    void solver::solve_inconsistencies()
    {
        LOG_DEBUG("[" << get_name() << "] Solving inconsistencies");

        auto incs = get_incs(); // all the current inconsistencies..
        LOG_DEBUG("[" << get_name() << "] Found " << incs.size() << " inconsistencies");

        // we solve the inconsistencies..
        while (!incs.empty())
        {
            if (const auto &uns_inc = std::find_if(incs.cbegin(), incs.cend(), [](const auto &v)
                                                   { return v.empty(); });
                uns_inc != incs.cend())
            { // we have an unsolvable inconsistency..
                LOG_DEBUG("[" << get_name() << "] Unsatisfiable inconsistency");
                next(); // we move to the next state..
            }
            else
            { // we check if we have a trivial inconsistencies..
                std::vector<utils::lit> trivial;
                for (const auto &inc : incs)
                    if (inc.size() == 1)
                        trivial.push_back(inc.front().first);
                if (!trivial.empty()) // we have trivial inconsistencies. We can learn something from them..
                    for (const auto &l : trivial)
                    {
                        LOG_DEBUG("[" << get_name() << "] Trivial inconsistency: " << to_string(l));
                        assert(get_sat().value(l) == utils::Undefined);
                        std::vector<utils::lit> learnt;
                        learnt.push_back(l);
                        for (const auto &l : get_sat().get_decisions())
                            learnt.push_back(!l);
                        gr.record(std::move(learnt));
                        if (!get_sat().propagate())
                            throw riddle::unsolvable_exception();
                    }
                else
                { // we have a non-trivial inconsistencies, so we have to take a decision..
                    std::vector<std::pair<utils::lit, double>> bst_inc;
                    double k_inv = std::numeric_limits<double>::infinity();
                    for (const auto &inc : incs)
                    {
                        double bst_commit = std::numeric_limits<double>::infinity();
                        for ([[maybe_unused]] const auto &[choice, commit] : inc)
                            if (commit < bst_commit)
                                bst_commit = commit;
                        double c_k_inv = 0;
                        for ([[maybe_unused]] const auto &[choice, commit] : inc)
                            c_k_inv += 1l / (1l + (commit - bst_commit));
                        if (c_k_inv < k_inv)
                        {
                            k_inv = c_k_inv;
                            bst_inc = inc;
                        }
                    }

                    // we select the best choice (i.e. the least committing one) from those available for the best flaw..
                    take_decision(std::min_element(bst_inc.cbegin(), bst_inc.cend(), [](const auto &ch0, const auto &ch1)
                                                   { return ch0.second < ch1.second; })
                                      ->first);
                }
            }

            incs = get_incs(); // we get the new inconsistencies..
            LOG_DEBUG("[" << get_name() << "] Found " << incs.size() << " inconsistencies");
        }

        LOG_DEBUG("[" << get_name() << "] Inconsistencies solved");
    }

    std::vector<std::vector<std::pair<utils::lit, double>>> solver::get_incs() const noexcept
    {
        std::vector<std::vector<std::pair<utils::lit, double>>> incs;
        for (const auto &st : smart_types)
        {
            auto st_incs = st->get_current_incs();
            incs.insert(incs.end(), st_incs.begin(), st_incs.end());
        }
        return incs;
    }

    void solver::reset_smart_types() noexcept
    {
        // we reset the smart types..
        smart_types.clear();
        // we seek for the existing smart types..
        std::queue<riddle::component_type *> q;
        for (const auto &t : get_types())
            if (auto ct = dynamic_cast<riddle::component_type *>(&t.get()))
                q.push(ct);
        while (!q.empty())
        {
            auto ct = q.front();
            q.pop();
            if (const auto st = dynamic_cast<smart_type *>(ct); st)
                smart_types.emplace_back(st);
            for (const auto &t : ct->get_types())
                if (auto c_ct = dynamic_cast<riddle::component_type *>(&t.get()))
                    q.push(c_ct);
        }
    }

    std::shared_ptr<riddle::item> solver::get(riddle::enum_item &enm, std::string_view name) noexcept { return enm.get(name); }
} // namespace ratio