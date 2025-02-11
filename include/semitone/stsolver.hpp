#pragma once

#include "graph.hpp"
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
    bool_item(riddle::bool_type &tp, const utils::lit &expr) : riddle::bool_item(tp), expr(expr) {}

    [[nodiscard]] const utils::lit &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lit expr;
  };

  class arith_item : public riddle::arith_item
  {
  public:
    arith_item(riddle::int_type &tp, const utils::lin &expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::real_type &tp, const utils::lin &expr) : riddle::arith_item(tp), expr(expr) {}
    arith_item(riddle::time_type &tp, const utils::lin &expr) : riddle::arith_item(tp), expr(expr) {}

    [[nodiscard]] const utils::lin &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::lin expr;
  };

  class string_item : public riddle::string_item
  {
  public:
    string_item(riddle::string_type &tp, const std::string &expr) : riddle::string_item(tp), expr(expr) {}

    [[nodiscard]] const std::string &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    const std::string expr;
  };

  class enum_item : public riddle::enum_item
  {
  public:
    enum_item(riddle::type &tp, utils::var expr, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) : riddle::enum_item(tp, std::move(values)), expr(expr) {}

    [[nodiscard]] const utils::var &get_expr() const noexcept { return expr; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    const utils::var expr;
  };

  class atom : public riddle::atom
  {
  public:
    atom(statom_flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args);

    [[nodiscard]] statom_flaw &get_flaw() noexcept { return flaw; }

    [[nodiscard]] const utils::lit &get_sigma() const noexcept { return sigma; }

    [[nodiscard]] riddle::bool_expr operator==(riddle::expr rhs) const override;

    [[nodiscard]] riddle::atom_state get_state() const override;

    [[nodiscard]] json::json to_json() const override;

  private:
    statom_flaw &flaw;      // the flaw associated with this atom..
    const utils::lit sigma; // the activation status of the atom (i.e., False if inactive, True if active, Undefined if unified)....
  };

  class stsolver : public graph
  {
    friend class bool_item;
    friend class arith_item;
    friend class string_item;
    friend class enum_item;
    friend class atom;
    friend class stflaw;

  public:
    stsolver(std::string_view name = "oRatio") noexcept;
    virtual ~stsolver() = default;
  };
} // namespace ratio
