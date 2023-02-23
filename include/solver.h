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
#include "flaw.h"
#include "resolver.h"

namespace ratio
{
  class flaw;
  class resolver;
#ifdef BUILD_LISTENERS
  class solver_listener;
#endif

  class solver : public riddle::core, public semitone::theory
  {
    friend class flaw;
    friend class resolver;
#ifdef BUILD_LISTENERS
    friend class solver_listener;
#endif

  public:
    solver();

    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return semitone::lra_theory& The linear-real-arithmetic theory.
     */
    semitone::lra_theory &get_lra_theory() { return lra_th; }
    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return const semitone::lra_theory& The linear-real-arithmetic theory.
     */
    const semitone::lra_theory &get_lra_theory() const { return lra_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return semitone::ov_theory& The object-variable theory.
     */
    semitone::ov_theory &get_ov_theory() { return ov_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return const semitone::ov_theory& The object-variable theory.
     */
    const semitone::ov_theory &get_ov_theory() const { return ov_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return semitone::idl_theory& The integer difference logic theory.
     */
    semitone::idl_theory &get_idl_theory() { return idl_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return const semitone::idl_theory& The integer difference logic theory.
     */
    const semitone::idl_theory &get_idl_theory() const { return idl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return semitone::rdl_theory& The real difference logic theory.
     */
    semitone::rdl_theory &get_rdl_theory() { return rdl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return const semitone::rdl_theory& The real difference logic theory.
     */
    const semitone::rdl_theory &get_rdl_theory() const { return rdl_th; }

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

    riddle::expr new_item(riddle::complex_type &tp) override;

    riddle::expr new_enum(riddle::type &tp, const std::vector<riddle::expr> &xprs) override;
    riddle::expr get_enum(riddle::expr &xpr, const std::string &name) override;

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

    void new_disjunction(std::vector<riddle::conjunction_ptr> xprs) override;

    riddle::expr new_fact(const riddle::predicate &pred) override;
    riddle::expr new_goal(const riddle::predicate &pred) override;

    bool is_constant(const riddle::expr &xpr) const override;

    utils::lbool bool_value(const riddle::expr &xpr) const override;
    utils::inf_rational arith_value(const riddle::expr &xpr) const override;
    utils::inf_rational time_value(const riddle::expr &xpr) const override;

    bool is_enum(const riddle::expr &xpr) const override;
    std::vector<riddle::expr> domain(const riddle::expr &xpr) const override;
    void prune(const riddle::expr &xpr, const riddle::expr &val) override;

  private:
    void new_flaw(utils::u_ptr<flaw> f, const bool &enqueue = true); // notifies the solver that a new flaw `f` has been created..
    void new_resolver(utils::u_ptr<resolver> r);                     // notifies the solver that a new resolver `r` has been created..

    inline const std::vector<std::reference_wrapper<resolver>> get_cause()
    {
      if (res)
        return {*res};
      else
        return {};
    }

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

    resolver *res = nullptr; // the current resolver (i.e. the cause for the new flaws)..

    std::unordered_map<semitone::var, std::vector<utils::u_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::unordered_map<semitone::var, std::vector<utils::u_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..

#ifdef BUILD_LISTENERS
  private:
    std::vector<solver_listener *> listeners; // the solver listeners..

    void fire_new_flaw(const flaw &f) const;
    void fire_flaw_state_changed(const flaw &f) const;
    void fire_flaw_cost_changed(const flaw &f) const;
    void fire_current_flaw(const flaw &f) const;
    void fire_new_resolver(const resolver &r) const;
    void fire_resolver_state_changed(const resolver &r) const;
    void fire_current_resolver(const resolver &r) const;
    void fire_causal_link_added(const flaw &f, const resolver &r) const;
#endif
  };

  json::json value(const riddle::item &itm) noexcept;
  inline json::json to_json(const utils::rational &rat) noexcept
  {
    json::json j_rat;
    j_rat["num"] = rat.numerator();
    j_rat["den"] = rat.denominator();
    return j_rat;
  }
  inline json::json to_json(const utils::inf_rational &rat) noexcept
  {
    json::json j_rat = to_json(rat.get_rational());
    if (rat.get_infinitesimal() != utils::rational::ZERO)
      j_rat["inf"] = to_json(rat.get_infinitesimal());
    return j_rat;
  }
} // namespace ratio
