#pragma once

#include <unordered_set>
#include "solver_item.hpp"
#include "core.hpp"
#include "lra_theory.hpp"
#include "idl_theory.hpp"
#include "rdl_theory.hpp"
#include "ov_theory.hpp"
#include "graph.hpp"

namespace ratio
{
  class solver : public riddle::core, public semitone::theory
  {
  public:
    solver(const std::string &name = "oRatio", bool i = true) noexcept;
    virtual ~solver() = default;

    void init() noexcept;

    semitone::lra_theory &get_lra_theory() noexcept { return lra; }
    const semitone::lra_theory &get_lra_theory() const noexcept { return lra; }
    semitone::idl_theory &get_idl_theory() noexcept { return idl; }
    const semitone::idl_theory &get_idl_theory() const noexcept { return idl; }
    semitone::rdl_theory &get_rdl_theory() noexcept { return rdl; }
    const semitone::rdl_theory &get_rdl_theory() const noexcept { return rdl; }
    semitone::ov_theory &get_ov_theory() noexcept { return ov; }
    const semitone::ov_theory &get_ov_theory() const noexcept { return ov; }

    std::shared_ptr<riddle::item> new_bool() noexcept override;
    std::shared_ptr<riddle::item> new_bool(bool value) noexcept override;
    std::shared_ptr<riddle::item> new_int() noexcept override;
    std::shared_ptr<riddle::item> new_int(INTEGER_TYPE value) noexcept override;
    std::shared_ptr<riddle::item> new_real() noexcept override;
    std::shared_ptr<riddle::item> new_real(const utils::rational &value) noexcept override;
    std::shared_ptr<riddle::item> new_time() noexcept override;
    std::shared_ptr<riddle::item> new_time(const utils::rational &value) noexcept override;
    std::shared_ptr<riddle::item> new_string() noexcept override;
    std::shared_ptr<riddle::item> new_string(const std::string &value) noexcept override;
    std::shared_ptr<riddle::item> new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;

    std::shared_ptr<riddle::item> minus(const std::shared_ptr<riddle::item> &xpr) override;
    std::shared_ptr<riddle::item> add(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> sub(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> mul(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> div(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;

    std::shared_ptr<riddle::item> lt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> leq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> gt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> geq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;

    std::shared_ptr<riddle::item> conj(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> disj(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> exct_one(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> negate(const std::shared_ptr<riddle::item> &expr) override;

    void assert_fact(const std::shared_ptr<riddle::item> &fact) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept override;

    std::shared_ptr<riddle::item> new_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept override;

    bool is_constant(const riddle::item &xpr) const noexcept
    {
      if (is_bool(xpr)) // the expression is a boolean..
        return bool_value(xpr) != utils::Undefined;
      else if (is_arith(xpr))
      { // the expression is an arithmetic value..
        auto [lb, ub] = bounds(xpr);
        return lb == ub;
      }
      else if (is_enum(xpr)) // the expression is an enumeration value..
        return ov.domain(static_cast<const enum_item &>(xpr).get_value()).size() == 1;
      else // the expression is a single value..
        return true;
    }

    utils::lbool bool_value(const riddle::item &expr) const noexcept override;
    utils::inf_rational arithmetic_value(const riddle::item &expr) const noexcept override;
    std::pair<utils::inf_rational, utils::inf_rational> bounds(const riddle::item &expr) const noexcept override;
    bool is_enum(const riddle::item &expr) const noexcept override { return dynamic_cast<const enum_item *>(&expr) != nullptr; }
    std::vector<std::reference_wrapper<utils::enum_val>> domain(const riddle::item &expr) const noexcept override;

  private:
    bool propagate(const utils::lit &) noexcept override { return true; }
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    std::string name;                        // the name of the solver
    semitone::lra_theory lra;                // the linear real arithmetic theory
    semitone::idl_theory idl;                // the integer difference logic theory
    semitone::rdl_theory rdl;                // the real difference logic theory
    semitone::ov_theory ov;                  // the object variable theory
    graph gr;                                // the causal graph
    std::optional<resolver> res;             // the current resolver
    std::unordered_set<flaw *> active_flaws; // the currently active flaws..
  };
} // namespace ratio
