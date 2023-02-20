#pragma once

#include "oratiosolver_export.h"
#include "core.h"
#include "item.h"
#include "theory.h"
#include "sat_stack.h"
#include "lra_theory.h"
#include "ov_theory.h"
#include "idl_theory.h"
#include "rdl_theory.h"

namespace ratio
{
  class solver : public riddle::core, public semitone::theory
  {
  public:
    solver();

    riddle::expr new_bool() override;
    riddle::expr new_bool(bool value) override;

    riddle::expr new_int() override;
    riddle::expr new_int(utils::I value) override;

    riddle::expr new_real() override;
    riddle::expr new_real(utils::rational value) override;

    riddle::expr new_time_point() override;
    riddle::expr new_time_point(utils::rational value) override;

    riddle::expr new_string() override;
    riddle::expr new_string(const std::string &value) override;

    riddle::expr new_enum(const riddle::type &tp, const std::vector<riddle::expr> &xprs) override;

    riddle::expr add(const std::vector<riddle::expr> &xprs) override;
    riddle::expr sub(const std::vector<riddle::expr> &xprs) override;
    riddle::expr mul(const std::vector<riddle::expr> &xprs) override;
    riddle::expr div(const std::vector<riddle::expr> &xprs) override;
    riddle::expr minus(const riddle::expr &xpr) override;

    riddle::expr lt(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr leq(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr gt(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr geq(const riddle::expr &lhs, const riddle::expr &rhs) override;
    riddle::expr eq(const riddle::expr &lhs, const riddle::expr &rhs) override;

    riddle::expr conj(const std::vector<riddle::expr> &xprs) override;
    riddle::expr disj(const std::vector<riddle::expr> &xprs) override;
    riddle::expr exct_one(const std::vector<riddle::expr> &xprs) override;
    riddle::expr negate(const riddle::expr &xpr) override;

    void assert_fact(const riddle::expr &xpr) override;

    void new_disjunction(const std::vector<riddle::conjunction_ptr> &xprs) override;

    riddle::expr new_fact(const riddle::predicate &pred) override;
    riddle::expr new_goal(const riddle::predicate &pred) override;

    utils::lbool bool_value(const riddle::expr &xpr) const override;
    utils::inf_rational arith_value(const riddle::expr &xpr) const override;
    utils::inf_rational time_value(const riddle::expr &xpr) const override;

    bool is_enum(const riddle::expr &xpr) const override;
    std::vector<riddle::expr> domain(const riddle::expr &xpr) const override;
    void prune(const riddle::expr &xpr, const riddle::expr &val) override;

  private:
    bool propagate(const semitone::lit &p) override;
    bool check() override;
    void push() override;
    void pop() override;

  private:
    semitone::lit tmp_ni;                  // the temporary controlling literal, used for restoring the controlling literal..
    semitone::lit ni = semitone::TRUE_lit; // the current controlling literal..

    semitone::lra_theory lra_th; // the linear-real-arithmetic theory..
    semitone::ov_theory ov_th;   // the object-variable theory..
    semitone::idl_theory idl_th; // the integer difference logic theory..
    semitone::rdl_theory rdl_th; // the real difference logic theory..
  };
} // namespace ratio
