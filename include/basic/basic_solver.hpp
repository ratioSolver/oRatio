#pragma once

#include "solver_core.hpp"
#include "items.hpp"

namespace ratio
{
  class flaw;
  class resolver;
  class atom_flaw;
  class solver;

  class atom : public riddle::atom
  {
  public:
    atom(riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma, atom_flaw &flaw) noexcept : riddle::atom(pred, is_fact, std::move(args), std::move(sigma)), flaw(flaw) {}

    [[nodiscard]] atom_flaw &get_flaw() noexcept { return flaw; }

  private:
    atom_flaw &flaw; // the flaw associated with this atom..
  };

  class node
  {
    friend class solver;

  public:
    node(std::optional<std::reference_wrapper<node>> parent = std::nullopt, std::shared_ptr<resolver> res = nullptr) noexcept;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    std::optional<std::reference_wrapper<node>> parent;   // The parent node..
    std::shared_ptr<resolver> res;                        // The resolver applied to
    std::unordered_set<std::shared_ptr<flaw>> open_flaws; // The set of open flaws..
  };

  class solver : public solver_core
  {
    friend class flaw;
    friend class resolver;
    static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  public:
    solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;

    void solve() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    [[nodiscard]] riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

    const node &find_common_ancestor(const node &a, const node &b) const;

    void backtrack_to(const node &lca) noexcept;

    [[nodiscard]] bool go_to(const node &target) noexcept;

    [[nodiscard]] bool apply_resolver(resolver &res) noexcept;

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
    virtual void node_created([[maybe_unused]] const node &n) noexcept {}
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
    virtual void flaw_created([[maybe_unused]] const node &n, [[maybe_unused]] const flaw &f) noexcept {}
    /**
     * @brief This function is called when the current node changes.
     *
     * This function should be overridden by derived classes to handle the event of a current node change.
     *
     * @param n The new current node.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void current_node([[maybe_unused]] std::optional<std::reference_wrapper<node>> n) noexcept {}
#endif

  private:
    std::vector<std::unique_ptr<node>> nodes;           // All nodes created during the solving process..
    std::optional<std::reference_wrapper<node>> c_node; // The current node in the search tree..
    std::vector<std::reference_wrapper<node>> fringe;   // The fringe of the search tree..
  };
} // namespace ratio
