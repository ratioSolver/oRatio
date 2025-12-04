#pragma once

#include "core.hpp"
#include "flaw.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"

namespace ratio
{
  static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  class solver;

  class flaw : public riddle::flaw
  {
    friend class solver;
    friend class resolver;

  public:
    flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes);

  private:
    virtual void compute_resolvers() = 0;
  };

  class resolver : public riddle::resolver
  {
    friend class solver;
    friend class flaw;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost);

  private:
    [[nodiscard]] virtual bool apply() noexcept = 0;

  protected:
    linspire::constraint lin_cnsts;                                            // The linear constraints in the current context..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints in the current context..
  };

  class solver : public riddle::core
  {
  public:
    solver(std::string_view name = "oRatio") noexcept;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(riddle::const_bool_expr expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_int() override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE value) override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE lb, const INT_TYPE ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) override;

    [[nodiscard]] riddle::arith_expr new_real() override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&value) override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&lb, utils::rational &&ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_real(utils::rational &&lb, utils::rational &&ub) override;

    [[nodiscard]] riddle::arith_expr new_time() override;
    [[nodiscard]] riddle::arith_expr new_time(utils::rational &&value) override;

    [[nodiscard]] utils::inf_rational arith_value(riddle::const_arith_expr expr) const noexcept override;
    bool is_constant(riddle::const_arith_expr expr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;
    [[nodiscard]] std::string string_value(riddle::const_string_expr expr) const noexcept override;

    [[nodiscard]] std::unordered_set<riddle::expr> enum_value(riddle::const_enum_expr expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::const_arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::const_arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_subtraction(std::vector<riddle::const_arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::const_arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_division(std::vector<riddle::const_arith_expr> &&xprs) override;

    riddle::atom_state get_atom_state(riddle::const_atom_expr atm) const noexcept override;

    virtual void solve() = 0;

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

  protected:
    void compute_resolvers(flaw &flw) { flw.compute_resolvers(); }
    bool apply_resolver(resolver &res) noexcept { return res.apply(); }

  private:
    bool mk_assign(riddle::const_bool_expr xpr, utils::lbool val) noexcept override;
    bool mk_eq(riddle::const_bool_expr lhs, riddle::const_bool_expr rhs) noexcept override;
    bool mk_neq(riddle::const_bool_expr lhs, riddle::const_bool_expr rhs) noexcept override;

    bool mk_lt(riddle::const_arith_expr lhs, riddle::const_arith_expr rhs) noexcept override;
    bool mk_le(riddle::const_arith_expr lhs, riddle::const_arith_expr rhs) noexcept override;
    bool mk_eq(riddle::const_arith_expr lhs, riddle::const_arith_expr rhs) noexcept override;
    bool mk_neq(riddle::const_arith_expr lhs, riddle::const_arith_expr rhs) noexcept override;

    bool mk_assign(riddle::const_enum_expr xpr, const utils::enum_val &val) noexcept override;
    bool mk_forbid(riddle::const_enum_expr xpr, const utils::enum_val &val) noexcept override;
    bool mk_eq(riddle::const_enum_expr lhs, riddle::const_enum_expr rhs) noexcept override;
    bool mk_neq(riddle::const_enum_expr lhs, riddle::const_enum_expr rhs) noexcept override;

  protected:
    arc_consistency::solver ac_slv;          // The arc consistency solver..
    linspire::solver lin_slv;                // The linear programming solver..
    std::shared_ptr<resolver> ctx = nullptr; // The current resolver context..
  };
} // namespace ratio
