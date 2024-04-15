#include <cassert>
#include <algorithm>
#include "solver.hpp"
#include "graph.hpp"
#include "init.hpp"
#include "smart_type.hpp"
#include "atom_flaw.hpp"
#include "bool_flaw.hpp"
#include "disj_flaw.hpp"
#include "disjunction_flaw.hpp"
#include "logging.hpp"

namespace ratio
{
    atom::atom(riddle::predicate &p, bool is_fact, atom_flaw &reason, std::map<std::string, std::shared_ptr<item>> &&args) : riddle::atom(p, is_fact, utils::lit(static_cast<solver &>(p.get_scope().get_core()).get_sat().new_var()), std::move(args)), reason(reason) {}

    solver::solver(const std::string &name) noexcept : name(name), sat(), lra(sat.new_theory<semitone::lra_theory>()), idl(sat.new_theory<semitone::idl_theory>()), rdl(sat.new_theory<semitone::rdl_theory>()), ov(sat.new_theory<semitone::ov_theory>()), gr(sat.new_theory<graph>(*this)) {}

    void solver::init()
    {
        LOG_DEBUG("[" << name << "] Initializing solver");
        // we read the init string..
        read(INIT_STRING);

        if (!sat.propagate())
            throw riddle::unsolvable_exception();
    }

    void solver::read(const std::string &script)
    {
        LOG_DEBUG("[" << name << "] Reading script: " << script);
        core::read(script);

        if (!sat.propagate())
            throw riddle::unsolvable_exception();
    }

    void solver::read(const std::vector<std::string> &files)
    {
        LOG_DEBUG("[" << name << "] Reading files");
        core::read(files);

        if (!sat.propagate())
            throw riddle::unsolvable_exception();
    }

    bool solver::solve()
    {
        LOG_DEBUG("[" << name << "] Solving problem");
        return true;
    }

    void solver::take_decision(const utils::lit &d)
    {
        assert(sat.value(d) == utils::Undefined);

        LOG_DEBUG("[" << name << "] Taking decision: " << to_string(d));
        // we take the decision..
        if (!sat.assume(d))
            throw riddle::unsolvable_exception();
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
    std::shared_ptr<riddle::enum_item> solver::new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) { return std::make_shared<riddle::enum_item>(tp, ov.new_var(std::move(values))); }

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
            return std::make_shared<riddle::bool_item>(get_bool_type(), utils::FALSE_lit);
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
        return std::make_shared<riddle::bool_item>(get_bool_type(), gr.new_flaw<disj_flaw>(*this, std::vector<std::reference_wrapper<resolver>>(), std::move(lits)).get_phi());
    }
    std::shared_ptr<riddle::bool_item> solver::exct_one(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs)
    {
        std::vector<utils::lit> lits;
        lits.reserve(exprs.size());
        for (const auto &xpr : exprs)
            lits.push_back(xpr->get_value());
        return std::make_shared<riddle::bool_item>(get_bool_type(), gr.new_flaw<disj_flaw>(*this, std::vector<std::reference_wrapper<resolver>>(), std::move(lits), true).get_phi());
    }
    std::shared_ptr<riddle::bool_item> solver::negate(const std::shared_ptr<riddle::bool_item> &expr) { return std::make_shared<riddle::bool_item>(get_bool_type(), !expr->get_value()); }

    void solver::assert_fact(const std::shared_ptr<riddle::bool_item> &fact)
    {
        if (!sat.new_clause({!gr.get_ni(), fact->get_value()}))
            throw riddle::unsolvable_exception();
    }

    void solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept { gr.new_flaw<disjunction_flaw>(*this, std::vector<std::reference_wrapper<resolver>>(), std::move(disjuncts)); }

    std::shared_ptr<riddle::atom> solver::new_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept
    {
        LOG_TRACE("Creating new " << pred.get_name() << " atom");
        auto atm = gr.new_flaw<atom_flaw>(*this, std::vector<std::reference_wrapper<resolver>>(), is_fact, pred, std::move(arguments)).get_atom();
        // we check if we need to notify any smart types of the new goal..
        if (!is_core(atm->get_type().get_scope()))
        {
            std::queue<riddle::component_type *> q;
            q.push(static_cast<riddle::component_type *>(&atm->get_type().get_scope())); // we start from the scope of the atom..
            while (!q.empty())
            {
                if (auto st = dynamic_cast<smart_type *>(q.front()))
                    st->new_atom(*atm);
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

    std::shared_ptr<riddle::item> solver::get(riddle::enum_item &enm, const std::string &name) noexcept { return enm.get(name); }

#ifdef ENABLE_VISUALIZATION
    json::json to_json(const riddle::item &itm) noexcept
    {
        json::json j_itm{{"id", get_id(itm)}, {"name", itm.get_type().get_scope().get_core().guess_name(itm)}};
        // we add the full name of the type of the item..
        std::string tp_name = itm.get_type().get_name();
        const auto *t = &itm.get_type();
        while (const auto *et = dynamic_cast<const riddle::type *>(&t->get_scope()))
        {
            tp_name.insert(0, et->get_name() + ".");
            t = et;
        }
        j_itm["type"] = tp_name;

        if (auto ci = dynamic_cast<const riddle::component *>(&itm))
        {
            json::json j_itms;
            for (const auto &[i_name, i] : ci->get_items())
                j_itms[i_name] = value(*i);
            if (j_itms.size())
                j_itm["exprs"] = std::move(j_itms);
        }

        if (auto a = dynamic_cast<const riddle::atom *>(&itm))
        {
            j_itm["sigma"] = variable(a->get_sigma());
            switch (static_cast<solver &>(a->get_type().get_scope().get_core()).get_sat().value(a->get_sigma()))
            {
            case utils::True:
                j_itm["status"] = "Active";
                break;
            case utils::False:
                j_itm["status"] = "Unified";
                break;
            default:
                j_itm["status"] = "Inactive";
                break;
            }
            j_itm["is_fact"] = a->is_fact();
            json::json j_itms;
            for (const auto &[i_name, i] : a->get_items())
                j_itms[i_name] = value(*i);
            if (j_itms.size())
                j_itm["exprs"] = std::move(j_itms);
        }

        return j_itm;
    }

    json::json value(const riddle::item &itm) noexcept
    {
        if (is_bool(itm))
        {
            auto &b_itm = static_cast<const riddle::bool_item &>(itm);
            json::json j_val{{"lit", (sign(b_itm.get_value()) ? "b" : "!b") + std::to_string(variable(b_itm.get_value()))}};
            switch (itm.get_type().get_scope().get_core().bool_value(b_itm))
            {
            case utils::True:
                j_val["val"] = "True";
                break;
            case utils::False:
                j_val["val"] = "False";
                break;
            default:
                j_val["val"] = "Undefined";
                break;
            }
            return j_val;
        }
        else if (is_arith(itm))
        {
            auto &a_itm = static_cast<const riddle::arith_item &>(itm);
            json::json j_val = to_json(itm.get_type().get_scope().get_core().arithmetic_value(a_itm));
            j_val["lin"] = to_string(a_itm.get_value());
            auto [lb, ub] = itm.get_type().get_scope().get_core().bounds(a_itm);
            if (!is_negative_infinite(lb))
                j_val["lb"] = to_json(lb);
            if (!is_positive_infinite(ub))
                j_val["ub"] = to_json(ub);
            return j_val;
        }
        else if (is_string(itm))
            return static_cast<const riddle::string_item &>(itm).get_value();
        else if (is_enum(itm))
        {
            auto &e_itm = static_cast<const riddle::enum_item &>(itm);
            json::json j_val{{"var", std::to_string(e_itm.get_value())}};
            json::json vals(json::json_type::array);
            for (auto &val : itm.get_type().get_scope().get_core().domain(e_itm))
                vals.push_back(get_id(dynamic_cast<riddle::item &>(val.get())));
            j_val["vals"] = std::move(vals);
            return j_val;
        }
        else
            return get_id(itm);
    }
#endif
} // namespace ratio