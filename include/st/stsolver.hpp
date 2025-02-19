#pragma once

#include "graph.hpp"
#include "item.hpp"
#include "semitone.hpp"

namespace ratio
{
  class stflaw;
  class statom_flaw;
  class stresolver;
  class stcomponent_type;
  class stunify_atom;

  class atom : public riddle::atom
  {
  public:
    atom(statom_flaw &flaw, riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept : riddle::atom(pred, is_fact, std::move(args), std::move(sigma)), flaw(flaw) {}

    [[nodiscard]] statom_flaw &get_flaw() noexcept { return flaw; }

  private:
    statom_flaw &flaw; // the flaw associated with this atom..
  };

  using atom_expr = utils::s_ptr<atom>;

  class stsolver : public graph, public smt::semitone
  {
    friend class stnetwork;
    friend class bool_item;
    friend class arith_item;
    friend class string_item;
    friend class enum_item;
    friend class atom;
    friend class stflaw;
    friend class stresolver;
    friend class stunify_atom;

  public:
    stsolver(std::string_view name = "oRatio") noexcept;
    virtual ~stsolver() = default;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_term &expr) const noexcept override;

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

    [[nodiscard]] utils::inf_rational arith_value(const riddle::arith_term &expr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;
    [[nodiscard]] std::string string_value(const riddle::string_term &expr) const noexcept override;

    [[nodiscard]] riddle::enum_expr new_enum(riddle::type &tp, std::vector<utils::ref_wrapper<utils::enum_val>> &&values) override;
    [[nodiscard]] std::vector<utils::ref_wrapper<utils::enum_val>> enum_value(const riddle::enum_term &expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_subtraction(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_division(std::vector<riddle::arith_expr> &&xprs) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<utils::u_ptr<riddle::conjunction>> &&disjuncts) override;

    void solve();

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

  private:
    riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

    void added_causal_link(flaw &f, resolver &r) override;

    void make_eq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p);
    void make_neq(riddle::term &lhs, riddle::term &rhs, const utils::lit &p);

    void pushed() noexcept override;
    void popped() noexcept override;
  };
} // namespace ratio
