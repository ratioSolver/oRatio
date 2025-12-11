#pragma once

#include "solver.hpp"
#include "types.hpp"

namespace ratio
{
  class local_search_solver : public solver
  {
  public:
    local_search_solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&) override;
    void new_clause(std::vector<riddle::bool_expr> &&) override;

    void solve() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::term>, std::less<>> &&args) override;

#ifdef ORATIO_ENABLE_LISTENERS
  private:
    /**
     * @brief This function is called when the state of the solver changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() {}
    /**
     * @brief Notifies when a flaw has been created.
     *
     * This function is called when a flaw has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw is created.
     *
     * @param flaw The flaw that has been created.
     */
    virtual void flaw_created(const riddle::flaw &) {}
    /**
     * @brief Notifies when the state of a flaw has changed.
     *
     * This function is called when the state of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's state changes.
     *
     * @param flaw The flaw whose state has changed.
     */
    virtual void flaw_state_changed(const riddle::flaw &) {}
    /**
     * @brief Notifies when the cost of a flaw has changed.
     *
     * This function is called when the cost of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's cost changes.
     *
     * @param flaw The flaw whose cost has changed.
     */
    virtual void flaw_cost_changed(const riddle::flaw &) {}
    /**
     * @brief Notifies when the current flaw has changed.
     *
     * This function is called when the current flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current flaw changes.
     *
     * @param flaw The current flaw.
     */
    virtual void current_flaw(std::shared_ptr<riddle::flaw>) {}

    /**
     * @brief Notifies when a resolver has been created.
     *
     * This function is called when a resolver has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver is created.
     *
     * @param resolver The resolver that has been created.
     */
    virtual void resolver_created(const riddle::resolver &) {}
    /**
     * @brief Notifies when the state of a resolver has changed.
     *
     * This function is called when the state of a resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver's state changes.
     *
     * @param resolver The resolver whose state has changed.
     */
    virtual void resolver_state_changed(const riddle::resolver &) {}
    /**
     * @brief Notifies when the current resolver has changed.
     *
     * This function is called when the current resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current resolver changes.
     *
     * @param resolver The current resolver.
     */
    virtual void current_resolver(std::shared_ptr<riddle::resolver>) {}

    /**
     * @brief Notifies when a causal link has been added.
     *
     * This function is called when a causal link has been added. It is a virtual function that can be overridden by derived classes to perform specific actions when a causal link is added.
     *
     * @param flaw The flaw that is the source of the causal link.
     * @param resolver The resolver that is the destination of the causal link.
     */
    virtual void causal_link_added(const riddle::flaw &, const riddle::resolver &) {}
#endif
  };

  class state_variable final : public riddle::state_variable
  {
  public:
    state_variable(local_search_solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_peak(std::vector<riddle::atom_expr> &&atms) noexcept override;
  };

  class reusable_resource final : public riddle::reusable_resource
  {
  public:
    reusable_resource(local_search_solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_peak(std::vector<riddle::atom_expr> &&atms) noexcept override;
  };

  class consumable_resource final : public riddle::consumable_resource
  {
  public:
    consumable_resource(local_search_solver &slv) noexcept;

  private:
    std::shared_ptr<riddle::flaw> new_overproduction(std::vector<riddle::atom_expr> &&prod_atms, std::vector<riddle::atom_expr> &&cons_atms) noexcept override;
    std::shared_ptr<riddle::flaw> new_overconsumption(std::vector<riddle::atom_expr> &&cons_atms, std::vector<riddle::atom_expr> &&prod_atms) noexcept override;
  };
} // namespace ratio
