#pragma once

#include "graph.hpp"
#include "mathsat.h"

namespace ratio
{
  class bool_item : public riddle::bool_item
  {
  public:
    bool_item(riddle::bool_type &tp, msat_term expr) : riddle::bool_item(tp), expr(expr) {}

    [[nodiscard]] msat_term &get_expr() noexcept { return expr; }
    [[nodiscard]] const msat_term &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    msat_term expr;
  };

  class arith_item : public riddle::arith_item
  {
  public:
    arith_item(riddle::int_type &tp, msat_term expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::real_type &tp, msat_term expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::time_type &tp, msat_term expr) : riddle::arith_item(tp), expr(expr) {}

    [[nodiscard]] msat_term &get_expr() noexcept { return expr; }
    [[nodiscard]] const msat_term &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    msat_term expr;
  };

  class string_item : public riddle::string_item
  {
  public:
    string_item(riddle::string_type &tp, const std::string &expr) : riddle::string_item(tp), expr(expr) {}

    [[nodiscard]] std::string &get_expr() noexcept { return expr; }
    [[nodiscard]] const std::string &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    std::string expr;
  };

  class enum_item : public riddle::enum_item
  {
  public:
    enum_item(riddle::type &tp, msat_term expr, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) : riddle::enum_item(tp, std::move(values)), expr(expr) {}

    [[nodiscard]] msat_term &get_expr() noexcept { return expr; }
    [[nodiscard]] const msat_term &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    msat_term expr;
  };

  class atom : public riddle::atom
  {
  public:
    atom(z3atom_flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args);

    [[nodiscard]] z3atom_flaw &get_flaw() noexcept { return flaw; }

    [[nodiscard]] msat_term &get_sigma() noexcept { return sigma; }
    [[nodiscard]] const msat_term &get_sigma() const noexcept { return sigma; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] riddle::atom_state get_state() const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    z3atom_flaw &flaw; // the flaw associated with this atom..
    msat_term sigma;   // the activation status of the atom (i.e., 0 if inactive, 1 if active, 2 if unified)....
  };

  using atom_expr = utils::s_ptr<atom>;

  class msatsolver : public graph
  {
    friend class bool_item;

  public:
    msatsolver(std::string_view name = "oRatio");

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
    [[nodiscard]] std::string string_value(const riddle::string_item &expr) const noexcept override;

    [[nodiscard]] riddle::enum_expr new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) override;
    [[nodiscard]] std::vector<utils::ref_wrapper<utils::enum_val>> enum_value(const riddle::enum_item &expr) const noexcept override;

    [[nodiscard]] riddle::bool_expr new_and(std::vector<riddle::bool_expr> &&exprs) override;
    [[nodiscard]] riddle::bool_expr new_or(std::vector<riddle::bool_expr> &&exprs) override;
    [[nodiscard]] riddle::bool_expr new_xor(std::vector<riddle::bool_expr> &&exprs) override;

    [[nodiscard]] riddle::bool_expr new_not(riddle::bool_expr expr) override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_subtraction(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_division(std::vector<riddle::arith_expr> &&xprs) override;

    [[nodiscard]] riddle::bool_expr new_lt(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_le(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_gt(riddle::arith_expr lhs, riddle::arith_expr rhs) override;
    [[nodiscard]] riddle::bool_expr new_ge(riddle::arith_expr lhs, riddle::arith_expr rhs) override;

    void new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) override;
    void assert_fact(riddle::bool_expr fact) override;

    bool solve();

  protected:
    virtual riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

  private:
    void expanded_flaw(flaw &f) override;

    void added_causal_link(flaw &f, resolver &r) override;

  private:
    msat_config cfg;
    msat_env env;
    msat_model mdl;
    size_t bool_count = 0;
    size_t int_count = 0;
    size_t real_count = 0;
    size_t time_count = 0;
    size_t string_count = 0;
    size_t enum_count = 0;
    size_t atom_count = 0;
    size_t position_count = 0;
  };
} // namespace ratio
