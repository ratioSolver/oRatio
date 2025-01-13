#include "z3solver.hpp"
#include "z3flaws.hpp"
#include "z3types.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

namespace ratio
{
    riddle::bool_expr bool_item::operator==(riddle::expr rhs) const
    {
        auto &ctx = static_cast<z3solver &>(get_type().get_scope()).ctx;
        if (rhs.get() == this) // same item
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(true));
        else if (rhs->get_type().get_name() != riddle::bool_kw) // different types
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        else if (auto b = std::dynamic_pointer_cast<bool_item>(rhs)) // same type
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), expr == b->get_expr());
        else if (auto e = std::dynamic_pointer_cast<enum_item>(rhs)) // enum with bool values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < e->get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(e->get_expr() == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const bool_item &>(e->get_values()[i].get()).get_expr() == expr);
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else
            assert(false);
    }

    riddle::bool_expr arith_item::operator==(riddle::expr rhs) const
    {
        auto &ctx = static_cast<z3solver &>(get_type().get_scope()).ctx;
        if (rhs.get() == this) // same item
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(true));
        else if (!rhs->get_type().is_assignable_from(get_type()) || !get_type().is_assignable_from(rhs->get_type())) // different types
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        else if (auto i = std::dynamic_pointer_cast<arith_item>(rhs)) // same type
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), expr == i->get_expr());
        else if (auto e = std::dynamic_pointer_cast<enum_item>(rhs)) // enum with arith values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < e->get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(e->get_expr() == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const arith_item &>(e->get_values()[i].get()).get_expr() == expr);
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else
            assert(false);
    }

    riddle::bool_expr string_item::operator==(riddle::expr rhs) const
    {
        auto &ctx = static_cast<z3solver &>(get_type().get_scope()).ctx;
        if (rhs.get() == this) // same item
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(true));
        else if (rhs->get_type().get_name() != riddle::string_kw) // different types
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        else if (auto s = std::dynamic_pointer_cast<string_item>(rhs)) // same type
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), expr == s->get_expr());
        else if (auto e = std::dynamic_pointer_cast<enum_item>(rhs)) // enum with string values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < e->get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(e->get_expr() == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const string_item &>(e->get_values()[i].get()).get_expr() == expr);
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else
            assert(false);
    }

    riddle::bool_expr enum_item::operator==(riddle::expr rhs) const
    {
        auto &ctx = static_cast<z3solver &>(get_type().get_scope()).ctx;
        if (rhs.get() == this) // same item
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(true));
        else if (!rhs->get_type().is_assignable_from(get_type()) || !get_type().is_assignable_from(rhs->get_type())) // different types
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        else if (auto b = std::dynamic_pointer_cast<bool_item>(rhs)) // bool with enum values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(expr == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const bool_item &>(get_values()[i].get()).get_expr() == b->get_expr());
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else if (auto a = std::dynamic_pointer_cast<arith_item>(rhs)) // arith with enum values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(expr == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const arith_item &>(get_values()[i].get()).get_expr() == a->get_expr());
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else if (auto s = std::dynamic_pointer_cast<string_item>(rhs)) // string with enum values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < get_values().size(); i++)
            {
                z3::expr_vector eq(ctx);
                eq.push_back(expr == ctx.int_val(static_cast<uint64_t>(i)));
                eq.push_back(static_cast<const string_item &>(get_values()[i].get()).get_expr() == s->get_expr());
                eqs.push_back(z3::mk_and(eq));
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else if (auto e = std::dynamic_pointer_cast<enum_item>(rhs)) // both enums with common values (otherwise handled by type check)
        {
            z3::expr_vector eqs(ctx);
            for (size_t i = 0; i < get_values().size(); i++)
                for (size_t j = 0; j < e->get_values().size(); j++)
                    if (&get_values()[i].get() == &e->get_values()[j].get())
                    {
                        z3::expr_vector eq(ctx);
                        eq.push_back(expr == ctx.int_val(static_cast<uint64_t>(i)));
                        eq.push_back(e->get_expr() == ctx.int_val(static_cast<uint64_t>(j)));
                        eqs.push_back(z3::mk_and(eq));
                    }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_or(eqs));
        }
        else
        {
            auto it = std::find_if(get_values().begin(), get_values().end(), [&rhs](const auto &v)
                                   { return &v.get() == rhs.get(); });
            if (it != get_values().end())
                return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), expr == ctx.int_val(static_cast<uint64_t>(std::distance(get_values().begin(), it))));
            else
                return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        }
    }

    atom::atom(z3flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args) : riddle::atom(pred, is_fact, std::move(args)), flaw(flaw), sigma(static_cast<z3solver &>(get_core()).ctx.int_const(("a" + std::to_string(static_cast<z3solver &>(get_core()).atom_count++)).c_str()))
    {
        static_cast<z3solver &>(get_core()).slv.add(sigma >= static_cast<z3solver &>(get_core()).ctx.int_val(0));
        static_cast<z3solver &>(get_core()).slv.add(sigma < static_cast<z3solver &>(get_core()).ctx.int_val(2));
    }
    riddle::bool_expr atom::operator==(riddle::expr rhs) const
    {
        auto &ctx = static_cast<z3solver &>(get_core()).ctx;
        if (rhs.get() == this) // same atom
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(true));
        else if (!rhs->get_type().is_assignable_from(get_type()) || !get_type().is_assignable_from(rhs->get_type())) // different predicates
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), ctx.bool_val(false));
        else if (auto a = std::dynamic_pointer_cast<atom>(rhs)) // same predicate
        {
            z3::expr_vector eqs(ctx);
            std::queue<const riddle::predicate *> q;
            q.push(&static_cast<const riddle::predicate &>(get_type()));
            while (!q.empty())
            {
                auto pred = q.front();
                q.pop();
                for (const auto &arg : pred->get_args())
                    eqs.push_back(std::dynamic_pointer_cast<bool_item>(get_core().new_eq(items.at(arg.get().get_name()), a->items.at(arg.get().get_name())))->get_expr());

                for (const auto &p : pred->get_parents())
                    q.push(&p.get());
            }
            return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type()), z3::mk_and(eqs));
        }
        else
            assert(false);
    }
    bool atom::is_active() const noexcept { return static_cast<z3solver &>(get_core()).mdl.eval(sigma, true).get_numeral_int() == 1; }

    z3solver::z3solver(std::string_view name) : graph(name), slv(ctx), mdl(ctx)
    {
        add_type(std::make_unique<z3state_variable>(*this));
        add_type(std::make_unique<z3reusable_resource>(*this));
    }

    riddle::bool_expr z3solver::new_bool() { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), ctx.bool_const(("b" + std::to_string(bool_count++)).c_str())); }
    riddle::bool_expr z3solver::new_bool(const bool value) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), ctx.bool_val(value)); }
    utils::lbool z3solver::bool_value(const riddle::bool_item &expr) const noexcept
    {
        switch (mdl.eval(static_cast<const bool_item &>(expr).get_expr(), true).bool_value())
        {
        case Z3_L_TRUE:
            return utils::True;
        case Z3_L_FALSE:
            return utils::False;
        default:
            return utils::Undefined;
        }
    }

    riddle::arith_expr z3solver::new_int() { return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), ctx.int_const(("i" + std::to_string(int_count++)).c_str())); }
    riddle::arith_expr z3solver::new_int(const INT_TYPE value) { return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), ctx.int_val(static_cast<int64_t>(value))); }
    riddle::arith_expr z3solver::new_int(const INT_TYPE lb, const INT_TYPE ub)
    {
        auto xpr = ctx.int_const(("i" + std::to_string(int_count++)).c_str());
        slv.add(xpr >= ctx.int_val(static_cast<int64_t>(lb)));
        slv.add(xpr <= ctx.int_val(static_cast<int64_t>(ub)));
        return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(xpr));
    }
    riddle::arith_expr z3solver::new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub)
    {
        auto xpr = ctx.int_const(("i" + std::to_string(int_count++)).c_str());
        slv.add(xpr >= ctx.int_val(static_cast<int64_t>(lb)));
        slv.add(xpr <= ctx.int_val(static_cast<int64_t>(ub)));
        return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), std::move(xpr));
    }

    riddle::arith_expr z3solver::new_real() { return std::make_shared<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), ctx.real_const(("r" + std::to_string(real_count++)).c_str())); }
    riddle::arith_expr z3solver::new_real(utils::rational &&value) { return std::make_shared<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), ctx.real_val(value.numerator(), value.denominator())); }
    riddle::arith_expr z3solver::new_real(utils::rational &&lb, utils::rational &&ub)
    {
        auto xpr = ctx.real_const(("r" + std::to_string(real_count++)).c_str());
        slv.add(xpr >= ctx.real_val(lb.numerator(), lb.denominator()));
        slv.add(xpr <= ctx.real_val(ub.numerator(), ub.denominator()));
        return std::make_shared<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(xpr));
    }
    riddle::arith_expr z3solver::new_uncertain_real(utils::rational &&lb, utils::rational &&ub)
    {
        auto xpr = ctx.real_const(("r" + std::to_string(real_count++)).c_str());
        slv.add(xpr >= ctx.real_val(lb.numerator(), lb.denominator()));
        slv.add(xpr <= ctx.real_val(ub.numerator(), ub.denominator()));
        return std::make_shared<arith_item>(static_cast<riddle::real_type &>(get_type(riddle::real_kw)), std::move(xpr));
    }

    riddle::arith_expr z3solver::new_time() { return std::make_shared<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), ctx.real_const(("t" + std::to_string(time_count++)).c_str())); }
    riddle::arith_expr z3solver::new_time(utils::rational &&value) { return std::make_shared<arith_item>(static_cast<riddle::time_type &>(get_type(riddle::time_kw)), ctx.real_val(value.numerator(), value.denominator())); }

    utils::inf_rational z3solver::arith_value(const riddle::arith_item &expr) const noexcept
    {
        auto val = mdl.eval(static_cast<const arith_item &>(expr).get_expr(), true);
        if (val.is_int())
            return utils::inf_rational(val.get_numeral_int());
        else if (val.is_real())
            return utils::inf_rational(val.numerator().get_numeral_int(), val.denominator().get_numeral_int());
        else
            assert(false);
    }

    riddle::string_expr z3solver::new_string() { return std::make_shared<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ctx.string_const(("s" + std::to_string(string_count++)).c_str())); }
    riddle::string_expr z3solver::new_string(std::string &&value) { return std::make_shared<string_item>(static_cast<riddle::string_type &>(get_type(riddle::string_kw)), ctx.string_val(value.c_str())); }

    riddle::enum_expr z3solver::new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values)
    {
        auto xpr = ctx.int_const(("e" + std::to_string(enum_count++)).c_str());
        slv.add(xpr >= ctx.int_val(0));
        slv.add(xpr < ctx.int_val(static_cast<uint64_t>(values.size())));
        return std::make_shared<enum_item>(static_cast<riddle::enum_type &>(tp), std::move(xpr), std::move(values));
    }
    utils::enum_val &z3solver::enum_value(const riddle::enum_item &expr) const noexcept
    {
        auto val = mdl.eval(static_cast<const enum_item &>(expr).get_expr(), true);
        return static_cast<utils::enum_val &>(expr.get_values()[val.get_numeral_uint()].get());
    }

    riddle::bool_expr z3solver::new_and(std::vector<riddle::bool_expr> &&exprs)
    {
        z3::expr_vector args(ctx);
        for (const auto &expr : exprs)
            args.push_back(static_cast<const bool_item &>(*expr).get_expr());
        return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), z3::mk_and(args));
    }
    riddle::bool_expr z3solver::new_or(std::vector<riddle::bool_expr> &&exprs)
    {
        z3::expr_vector args(ctx);
        for (const auto &expr : exprs)
            args.push_back(static_cast<const bool_item &>(*expr).get_expr());
        return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), z3::mk_or(args));
    }
    riddle::bool_expr z3solver::new_xor(std::vector<riddle::bool_expr> &&exprs)
    {
        z3::expr_vector args(ctx);
        for (const auto &expr : exprs)
            args.push_back(static_cast<const bool_item &>(*expr).get_expr());
        return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), z3::mk_xor(args));
    }

    riddle::bool_expr z3solver::new_not(riddle::bool_expr expr) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), !static_cast<const bool_item &>(*expr).get_expr()); }

    riddle::arith_expr z3solver::new_negation(riddle::arith_expr xpr) { return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), -static_cast<const arith_item &>(*xpr).get_expr()); }

    riddle::arith_expr z3solver::new_sum(std::vector<riddle::arith_expr> &&xprs)
    {
        z3::expr_vector args(ctx);
        for (const auto &xpr : xprs)
            args.push_back(static_cast<const arith_item &>(*xpr).get_expr());
        return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), z3::sum(args));
    }
    riddle::arith_expr z3solver::new_product(std::vector<riddle::arith_expr> &&xprs)
    {
        z3::expr_vector args(ctx);
        for (const auto &xpr : xprs)
            args.push_back(static_cast<const arith_item &>(*xpr).get_expr());
        z3::array<Z3_ast> _args(args);
        return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), to_expr(ctx, Z3_mk_mul(ctx, _args.size(), _args.ptr())));
    }
    riddle::arith_expr z3solver::new_divide(riddle::arith_expr lhs, riddle::arith_expr rhs) { return std::make_shared<arith_item>(static_cast<riddle::int_type &>(get_type(riddle::int_kw)), static_cast<const arith_item &>(*lhs).get_expr() / static_cast<const arith_item &>(*rhs).get_expr()); }

    riddle::bool_expr z3solver::new_lt(riddle::arith_expr lhs, riddle::arith_expr rhs) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const arith_item &>(*lhs).get_expr() < static_cast<const arith_item &>(*rhs).get_expr()); }
    riddle::bool_expr z3solver::new_le(riddle::arith_expr lhs, riddle::arith_expr rhs) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const arith_item &>(*lhs).get_expr() <= static_cast<const arith_item &>(*rhs).get_expr()); }
    riddle::bool_expr z3solver::new_gt(riddle::arith_expr lhs, riddle::arith_expr rhs) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const arith_item &>(*lhs).get_expr() > static_cast<const arith_item &>(*rhs).get_expr()); }
    riddle::bool_expr z3solver::new_ge(riddle::arith_expr lhs, riddle::arith_expr rhs) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const arith_item &>(*lhs).get_expr() >= static_cast<const arith_item &>(*rhs).get_expr()); }

    void z3solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        new_flaw<z3disjunction_flaw>(*this, std::move(causes), std::move(disjuncts));
    }
    void z3solver::assert_fact(riddle::bool_expr fact)
    {
        if (get_current_resolver().has_value())
            slv.add(z3::implies(static_cast<z3resolver &>(get_current_resolver().value().get()).get_rho(), static_cast<const bool_item &>(*fact).get_expr()));
        else
            slv.add(static_cast<const bool_item &>(*fact).get_expr());
    }

    riddle::atom_expr z3solver::create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args)
    {
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        auto &af = new_flaw<z3atom_flaw>(*this, std::move(causes), is_fact, pred, std::move(args));
        return af.get_atom();
    }

    bool z3solver::solve()
    {
        auto res = slv.check();
        if (res == z3::unsat)
            return false; // no solution..

        build(); // we build the causal graph..

        while (true)
        { // we try to solve the problem with the current causal graph..
            z3::expr_vector unexpanded_flaws(ctx);
            std::unordered_map<std::string, std::reference_wrapper<ratio::flaw>> flaw_map;
            for (const auto &flaw : get_queued_flaws())
            {
                unexpanded_flaws.push_back(static_cast<z3flaw &>(flaw.get()).get_phi());
                flaw_map.emplace(static_cast<z3flaw &>(flaw.get()).get_phi().to_string(), flaw);
            }
            while (true)
            {
                res = slv.check(unexpanded_flaws); // we check negating the unexpanded flaws..
                if (res == z3::sat)
                { // we found a solution with the current causal graph..
                    mdl = slv.get_model();
                    // if we find any inconsistency, we solve it..
                    bool inconsistencies = false;
                    std::queue<riddle::component_type *> q;
                    for (const auto &[_, tp] : get_types())
                        if (auto ctp = dynamic_cast<riddle::component_type *>(tp.get()))
                            q.push(ctp);
                    while (!q.empty())
                    {
                        auto tp = q.front();
                        q.pop();
                        if (auto z3tp = dynamic_cast<z3component_type *>(tp))
                            if (z3tp->solve_inconsistencies())
                                inconsistencies = true;
                        if (auto ctp = dynamic_cast<riddle::component_type *>(tp))
                            for (const auto &p : ctp->get_parents())
                                q.push(&p.get());
                    }
                    if (!inconsistencies)
                        return true; // solution found..
                }
                else if (res == z3::unknown)
                {
                    LOG_ERR("Z3 solver failed to solve the problem: " << slv.reason_unknown());
                    return false; // no solution..
                }
                else if (unexpanded_flaws.empty())
                    return false; // no solution..
                else
                { // we analyze the unsat core..
                    auto u_core = slv.unsat_core();
                    std::vector<std::reference_wrapper<ratio::flaw>> flaws;
                    for (const auto &f : u_core)
                        if (auto f_it = flaw_map.find(f.to_string()); f_it != flaw_map.end())
                            flaws.push_back(f_it->second);

                    if (!flaws.empty())
                    { // we expand the graph..
                        expand_flaws(flaws);
                        break;
                    }
                    else // the unsat core is within the current causal graph (i.e., no solution..)
                        return false;
                }
            }
        }

        return true;
    }

    void z3solver::expanded_flaw(flaw &f)
    {
        z3::expr_vector ress(ctx);
        for (const auto &resolver : f.get_resolvers())
            ress.push_back(static_cast<z3resolver &>(resolver.get()).get_rho());
        slv.add(z3::implies(static_cast<z3flaw &>(f).get_phi(), z3::mk_or(ress))); // if the flaw is active, then at least one resolver must be active
    }
} // namespace ratio