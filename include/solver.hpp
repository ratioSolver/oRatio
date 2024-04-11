#pragma once

#include <unordered_set>
#include "core.hpp"
#include "sat_core.hpp"
#include "lra_theory.hpp"
#include "idl_theory.hpp"
#include "rdl_theory.hpp"
#include "ov_theory.hpp"

namespace ratio
{
  class atom_flaw;
  class graph;

  class atom : public riddle::atom
  {
    friend class atom_flaw;

  public:
    atom(riddle::predicate &p, bool is_fact, atom_flaw &reason, std::map<std::string, std::shared_ptr<item>> &&args = {});

    [[nodiscard]] atom_flaw &get_reason() const { return reason; }

  private:
    atom_flaw &reason; // the flaw associated to this atom..
  };

  class solver : public riddle::core
  {
  public:
    solver(const std::string &name = "oRatio") noexcept;
    virtual ~solver() = default;

    const std::string &get_name() const noexcept { return name; }

    void init() noexcept;

    void read(const std::string &script) override;
    void read(const std::vector<std::string> &files) override;

    /**
     * @brief Solves the current problem returning whether a solution was found.
     *
     * @return true If a solution was found.
     * @return false If no solution was found.
     */
    bool solve();
    /**
     * @brief Takes a decision and propagates its consequences.
     *
     * @param d The decision to take.
     */
    void take_decision(const utils::lit &d);

    [[nodiscard]] semitone::sat_core &get_sat() noexcept { return sat; }
    [[nodiscard]] const semitone::sat_core &get_sat() const noexcept { return sat; }
    [[nodiscard]] semitone::lra_theory &get_lra_theory() noexcept { return lra; }
    [[nodiscard]] const semitone::lra_theory &get_lra_theory() const noexcept { return lra; }
    [[nodiscard]] semitone::idl_theory &get_idl_theory() noexcept { return idl; }
    [[nodiscard]] const semitone::idl_theory &get_idl_theory() const noexcept { return idl; }
    [[nodiscard]] semitone::rdl_theory &get_rdl_theory() noexcept { return rdl; }
    [[nodiscard]] const semitone::rdl_theory &get_rdl_theory() const noexcept { return rdl; }
    [[nodiscard]] semitone::ov_theory &get_ov_theory() noexcept { return ov; }
    [[nodiscard]] const semitone::ov_theory &get_ov_theory() const noexcept { return ov; }
    [[nodiscard]] graph &get_graph() noexcept { return gr; }
    [[nodiscard]] const graph &get_graph() const noexcept { return gr; }

    [[nodiscard]] std::shared_ptr<riddle::bool_item> new_bool() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_int() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_real() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> new_time() noexcept override;
    [[nodiscard]] std::shared_ptr<riddle::enum_item> new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;

    [[nodiscard]] std::shared_ptr<riddle::arith_item> minus(const std::shared_ptr<riddle::arith_item> &xpr) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> add(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> sub(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> mul(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;
    [[nodiscard]] std::shared_ptr<riddle::arith_item> div(const std::vector<std::shared_ptr<riddle::arith_item>> &xprs) override;

    [[nodiscard]] std::shared_ptr<riddle::bool_item> lt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> leq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> gt(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> geq(const std::shared_ptr<riddle::arith_item> &lhs, const std::shared_ptr<riddle::arith_item> &rhs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;

    [[nodiscard]] std::shared_ptr<riddle::bool_item> conj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> disj(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> exct_one(const std::vector<std::shared_ptr<riddle::bool_item>> &exprs) override;
    [[nodiscard]] std::shared_ptr<riddle::bool_item> negate(const std::shared_ptr<riddle::bool_item> &expr) override;

    void assert_fact(const std::shared_ptr<riddle::bool_item> &fact) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept override;

    [[nodiscard]] std::shared_ptr<riddle::atom> new_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments = {}) noexcept override;

    [[nodiscard]] utils::lbool bool_value(const riddle::bool_item &expr) const noexcept override;
    [[nodiscard]] utils::inf_rational arithmetic_value(const riddle::arith_item &expr) const noexcept override;
    [[nodiscard]] std::pair<utils::inf_rational, utils::inf_rational> bounds(const riddle::arith_item &expr) const noexcept override;
    [[nodiscard]] std::vector<std::reference_wrapper<utils::enum_val>> domain(const riddle::enum_item &expr) const noexcept override;
    [[nodiscard]] bool assign(const riddle::enum_item &expr, utils::enum_val &val) override;
    void forbid(const riddle::enum_item &expr, utils::enum_val &val) override;

  private:
    [[nodiscard]] std::shared_ptr<riddle::item> get(riddle::enum_item &enm, const std::string &name) noexcept override;

  private:
    const std::string name;    // the name of the solver
    semitone::sat_core sat;    // the SAT core
    semitone::lra_theory &lra; // the linear real arithmetic theory
    semitone::idl_theory &idl; // the integer difference logic theory
    semitone::rdl_theory &rdl; // the real difference logic theory
    semitone::ov_theory &ov;   // the object variable theory
    graph &gr;                 // the causal graph
  };
} // namespace ratio
