#pragma once

#include "graph.hpp"
#include "item.hpp"
#include "network.hpp"

namespace ratio
{
  class stflaw;
  class statom_flaw;
  class stresolver;
  class stcomponent_type;
  class stunify_atom;

  class bool_item : public riddle::bool_item
  {
  public:
    bool_item(riddle::bool_type &tp, utils::lit &&expr) : riddle::bool_item(tp, std::move(expr)) {}

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;
  };

  class arith_item : public riddle::arith_item
  {
  public:
    arith_item(riddle::int_type &tp, utils::lin &&expr) : riddle::arith_item(tp, std::move(expr)) {}
    arith_item(riddle::real_type &tp, utils::lin &&expr) : riddle::arith_item(tp, std::move(expr)) {}
    arith_item(riddle::time_type &tp, utils::lin &&expr) : riddle::arith_item(tp, std::move(expr)) {}

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;
  };

  class string_item : public riddle::string_item
  {
  public:
    string_item(riddle::string_type &tp, std::string &&expr) : riddle::string_item(tp, std::move(expr)) {}

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;
  };

  class enum_item : public riddle::enum_item
  {
  public:
    enum_item(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values, utils::var expr) : riddle::enum_item(tp, std::move(values), std::move(expr)) {}

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;
  };

  class atom : public riddle::atom
  {
  public:
    atom(statom_flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args);

    [[nodiscard]] statom_flaw &get_flaw() noexcept { return flaw; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    statom_flaw &flaw; // the flaw associated with this atom..
  };

  using atom_expr = utils::s_ptr<atom>;

  class stsolver : public graph
  {
    friend class bool_item;
    friend class arith_item;
    friend class string_item;
    friend class enum_item;
    friend class atom;
    friend class stflaw;
    friend class stresolver;

  public:
    stsolver(std::string_view name = "oRatio") noexcept;
    virtual ~stsolver() = default;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_itm &expr) const noexcept override;

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

    [[nodiscard]] utils::inf_rational arith_value(const riddle::arith_itm &expr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;
    [[nodiscard]] std::string string_value(const riddle::string_itm &expr) const noexcept override;

    [[nodiscard]] riddle::enum_expr new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) override;
    [[nodiscard]] std::vector<utils::ref_wrapper<utils::enum_val>> enum_value(const riddle::enum_itm &expr) const noexcept override;

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
    void assert_clause(std::vector<riddle::bool_expr> &&exprs) override;

    bool solve();

  private:
    riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

    void added_causal_link(flaw &f, resolver &r) override;

  private:
    semitone::network net;
  };
} // namespace ratio
