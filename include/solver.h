#pragma once

#include "oratio_export.h"
#include "core.h"
#include "sat_stack.h"
#include "lra_theory.h"
#include "ov_theory.h"
#include "idl_theory.h"
#include "rdl_theory.h"

namespace ratio::solver
{
  class causal_graph;
  class flaw;
  class resolver;

  class solver : public ratio::core::core
  {
  public:
    ORATIO_EXPORT solver();
    ORATIO_EXPORT solver(std::unique_ptr<causal_graph> gr);
    solver(const solver &orig) = delete;
    ORATIO_EXPORT ~solver();

    ORATIO_EXPORT ratio::core::expr new_bool() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_int() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_real() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_time_point() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_string() noexcept override;

    ORATIO_EXPORT ratio::core::expr new_enum(ratio::core::type &tp, const std::vector<ratio::core::expr> &allowed_vals) override;
    ORATIO_EXPORT ratio::core::expr get(ratio::core::enum_item &var, const std::string &name);
    ORATIO_EXPORT void remove(ratio::core::expr &var, ratio::core::expr &val) override;

    ORATIO_EXPORT ratio::core::expr negate(const ratio::core::expr &var) noexcept override;
    ORATIO_EXPORT ratio::core::expr conj(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr disj(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr exct_one(const std::vector<ratio::core::expr> &exprs) noexcept override;

    ORATIO_EXPORT ratio::core::expr add(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr sub(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr mult(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr div(const std::vector<ratio::core::expr> &exprs) noexcept override;
    ORATIO_EXPORT ratio::core::expr minus(const ratio::core::expr &ex) noexcept override;

    ORATIO_EXPORT ratio::core::expr lt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIO_EXPORT ratio::core::expr leq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIO_EXPORT ratio::core::expr eq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIO_EXPORT ratio::core::expr geq(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;
    ORATIO_EXPORT ratio::core::expr gt(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;

    ORATIO_EXPORT bool matches(const ratio::core::expr &left, const ratio::core::expr &right) noexcept override;

    inline semitone::lit get_ni() const noexcept { return ni; }
    inline void set_ni(const semitone::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    inline void restore_ni() noexcept { ni = tmp_ni; }

    ORATIO_EXPORT void new_disjunction(const std::vector<std::unique_ptr<ratio::core::conjunction>> conjs) override;

  private:
    ORATIO_EXPORT void new_atom(ratio::core::atom &atm, const bool &is_fact = true) override;

  public:
    ORATIO_EXPORT void assert_facts(std::vector<ratio::core::expr> facts) override;
    ORATIO_EXPORT void assert_facts(std::vector<semitone::lit> facts);

    /**
     * @brief Get the sat core.
     *
     * @return semitone::sat_core& The sat core.
     */
    inline semitone::sat_core &get_sat_core() noexcept { return sat_cr; }
    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return semitone::lra_theory& The linear-real-arithmetic theory.
     */
    inline semitone::lra_theory &get_lra_theory() noexcept { return lra_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return semitone::ov_theory& The object-variable theory.
     */
    inline semitone::ov_theory &get_ov_theory() noexcept { return ov_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return semitone::idl_theory& The integer difference logic theory.
     */
    inline semitone::idl_theory &get_idl_theory() noexcept { return idl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return semitone::rdl_theory& The real difference logic theory.
     */
    inline semitone::rdl_theory &get_rdl_theory() noexcept { return rdl_th; }

    inline size_t decision_level() const noexcept { return trail.size(); } // returns the current decision level..
    inline bool root_level() const noexcept { return trail.empty(); }      // checks whether the current decision level is root level..

    const causal_graph &get_causal_graph() const noexcept { return *gr; }
    const std::unordered_set<flaw *> &get_active_flaws() const noexcept { return active_flaws; }

    /**
     * Solves the given problem returning whether a solution has been found.
     */
    ORATIO_EXPORT bool solve();
    /**
     * Takes the given decision and propagates its effects.
     */
    ORATIO_EXPORT void take_decision(const semitone::lit &ch);

  private:
    semitone::lit tmp_ni;                  // the temporary controlling literal, used for restoring the controlling literal..
    semitone::lit ni = semitone::TRUE_lit; // the current controlling literal..

    semitone::sat_core sat_cr;   // the sat core..
    semitone::lra_theory lra_th; // the linear-real-arithmetic theory..
    semitone::ov_theory ov_th;   // the object-variable theory..
    semitone::idl_theory idl_th; // the integer difference logic theory..
    semitone::rdl_theory rdl_th; // the real difference logic theory..

    std::unique_ptr<causal_graph> gr;        // the causal graph..
    std::unordered_set<flaw *> active_flaws; // the currently active flaws..

    struct layer
    {
      std::unordered_map<flaw *, semitone::rational> old_f_costs; // the old estimated flaws' costs..
      std::unordered_set<flaw *> new_flaws;                       // the just activated flaws..
      std::unordered_set<flaw *> solved_flaws;                    // the just solved flaws..
    };
    std::vector<layer> trail; // the list of taken decisions, with the associated changes made, in chronological order..
  };
} // namespace ratio::solver
