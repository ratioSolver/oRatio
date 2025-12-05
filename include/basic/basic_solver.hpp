#pragma once

#include "solver.hpp"
#include "a_star.hpp"

namespace ratio
{
  class basic_solver;

  class node final : public utils::node<double>, public std::enable_shared_from_this<node>
  {
    friend class basic_solver;

  public:
    node(basic_solver &slv, std::weak_ptr<node> parent = {}) noexcept;
    node(const node &) = delete;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] virtual double cost(std::shared_ptr<utils::node<double>> goal = nullptr) const noexcept override;
    [[nodiscard]] virtual std::unordered_map<std::shared_ptr<utils::node<double>>, double> get_successors() override;
    [[nodiscard]] virtual bool is_goal() const noexcept override;

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    basic_solver &slv;                                              // The solver this node belongs to..
    bool consistent = true;                                         // Whether the node is consistent..
    std::weak_ptr<node> parent;                                     // The parent node..
    std::vector<std::shared_ptr<riddle::resolver>> resolvers;       // The resolvers applied within this node..
    std::unordered_set<std::shared_ptr<riddle::flaw>> open_flaws;   // The set of open flaws..
    std::unordered_set<std::shared_ptr<riddle::flaw>> closed_flaws; // The set of closed flaws..
  };

  class basic_solver : public solver, public utils::a_star<double>
  {
    friend class node;

  public:
    basic_solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&) override;
    void new_clause(std::vector<riddle::bool_expr> &&) override;

    void solve() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::term>, std::less<>> &&args) override;

    void retract(const utils::node<double> &) noexcept override {}
    bool expand(utils::node<double> &) noexcept override { return true; }

#ifdef ORATIO_ENABLE_LISTENERS
  private:
    /**
     * @brief This function is called when the state of the solver changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() noexcept {}
    /**
     * @brief This function is called when a new node is created.
     *
     * This function should be overridden by derived classes to handle the event of a new node creation.
     *
     * @param n The newly created node.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void node_created([[maybe_unused]] const utils::node<double> &n) noexcept {}
    /**
     * @brief This function is called when a new flaw is created on a node.
     *
     * This function should be overridden by derived classes to handle the event of a new flaw creation.
     *
     * @param n The node on which the flaw was created.
     * @param f The newly created flaw.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void flaw_created([[maybe_unused]] const utils::node<double> &n, [[maybe_unused]] const riddle::flaw &f) noexcept {}
    /**
     * @brief This function is called when a resolver is applied on a node.
     *
     * This function should be overridden by derived classes to handle the event of a resolver application.
     *
     * @param n The node on which the resolver was applied.
     * @param r The applied resolver.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void resolver_applied([[maybe_unused]] const utils::node<double> &n, [[maybe_unused]] const riddle::resolver &r) noexcept {}
    /**
     * @brief This function is called when the current node changes.
     *
     * This function should be overridden by derived classes to handle the event of a current node change.
     *
     * @param n The new current node.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void current_node([[maybe_unused]] const utils::node<double> &n) noexcept override {}
    /**
     * @brief This function is called when a node is found to be inconsistent.
     *
     * This function should be overridden by derived classes to handle the event of an inconsistent node.
     *
     * @param n The inconsistent node.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void inconsistent_node([[maybe_unused]] const utils::node<double> &n) noexcept override {}
#endif
  };
} // namespace ratio
