#include "z3solver.hpp"
#include "z3flaws.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio
{
    z3solver::z3solver() : slv(ctx), mdl(ctx) {}

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

    riddle::bool_expr z3solver::new_eq(std::shared_ptr<riddle::item> lhs, std::shared_ptr<riddle::item> rhs) { return std::make_shared<bool_item>(static_cast<riddle::bool_type &>(get_type(riddle::bool_kw)), static_cast<const arith_item &>(*lhs).get_expr() == static_cast<const arith_item &>(*rhs).get_expr()); }

    void z3solver::new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts)
    {
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
        auto xpr = ctx.int_const(("a" + std::to_string(atom_count++)).c_str());
        slv.add(xpr >= ctx.int_val(0));
        slv.add(xpr < ctx.int_val(2));
        auto atm = std::make_shared<atom>(pred, is_fact, std::move(xpr), std::move(args));
        std::vector<std::reference_wrapper<resolver>> causes;
        if (get_current_resolver().has_value())
            causes.push_back(get_current_resolver().value());
        auto &f = new_flaw<z3atom_flaw>(*this, std::move(causes), atm);
        return atm;
    }

    bool z3solver::solve()
    {
        auto res = slv.check();
        if (res == z3::sat)
        {
            mdl = slv.get_model();
            return true;
        }
        return false;
    }
} // namespace ratio