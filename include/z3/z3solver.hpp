#pragma once

#include "graph.hpp"
#include "c++/z3++.h"

namespace ratio
{
  class z3flaw;

  class bool_item : public riddle::bool_item
  {
  public:
    bool_item(riddle::bool_type &tp, z3::expr &&expr) : riddle::bool_item(tp), expr(expr) {}

    [[nodiscard]] z3::expr &get_expr() noexcept { return expr; }
    [[nodiscard]] const z3::expr &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

  private:
    z3::expr expr;
  };

  class arith_item : public riddle::arith_item
  {
  public:
    arith_item(riddle::int_type &tp, z3::expr &&expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::real_type &tp, z3::expr &&expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::time_type &tp, z3::expr &&expr) : riddle::arith_item(tp), expr(expr) {}

    [[nodiscard]] z3::expr &get_expr() noexcept { return expr; }
    [[nodiscard]] const z3::expr &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

  private:
    z3::expr expr;
  };

  class string_item : public riddle::string_item
  {
  public:
    string_item(riddle::string_type &tp, z3::expr &&expr) : riddle::string_item(tp), expr(expr) {}

    [[nodiscard]] z3::expr &get_expr() noexcept { return expr; }
    [[nodiscard]] const z3::expr &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

  private:
    z3::expr expr;
  };

  class enum_item : public riddle::enum_item
  {
  public:
    enum_item(riddle::type &tp, z3::expr &&expr, std::vector<std::reference_wrapper<utils::enum_val>> &&values) : riddle::enum_item(tp, std::move(values)), expr(expr) {}

    [[nodiscard]] z3::expr &get_expr() noexcept { return expr; }
    [[nodiscard]] const z3::expr &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

  private:
    z3::expr expr;
  };

  class atom : public riddle::atom
  {
  public:
    atom(riddle::predicate &pred, bool is_fact, z3::expr &&expr, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args) : riddle::atom(pred, is_fact, std::move(args)), expr(expr) {}

    [[nodiscard]] z3::expr &get_sigma() noexcept { return expr; }
    [[nodiscard]] const z3::expr &get_sigma() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

  private:
    z3::expr expr;
  };

  class z3solver : public graph
  {
    friend class bool_item;
    friend class arith_item;
    friend class string_item;
    friend class enum_item;
    friend class atom;
    friend class z3flaw;

  public:
    z3solver();

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_item &expr) const noexcept override;

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

    [[nodiscard]] utils::inf_rational arith_value(const riddle::arith_item &expr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;

    [[nodiscard]] riddle::enum_expr new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;

    [[nodiscard]] riddle::bool_expr new_and(std::vector<riddle::bool_expr> &&exprs) override;
    [[nodiscard]] riddle::bool_expr new_or(std::vector<riddle::bool_expr> &&exprs) override;
    [[nodiscard]] riddle::bool_expr new_xor(std::vector<riddle::bool_expr> &&exprs) override;

    [[nodiscard]] riddle::bool_expr new_not(riddle::bool_expr expr) override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_divide(riddle::arith_expr lhs, riddle::arith_expr rhs) override;

    [[nodiscard]] riddle::bool_expr new_lt(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_le(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_gt(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_ge(riddle::arith_expr lhs, riddle::arith_expr rhs) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;
    void assert_fact(riddle::bool_expr fact) override;

    bool solve();

  protected:
    virtual riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>, std::less<>> &&args) override;

  private:
    z3::context ctx;
    z3::solver slv;
    z3::model mdl;
    size_t bool_count = 0;
    size_t int_count = 0;
    size_t real_count = 0;
    size_t time_count = 0;
    size_t string_count = 0;
    size_t enum_count = 0;
    size_t atom_count = 0;
  };
} // namespace ratio
