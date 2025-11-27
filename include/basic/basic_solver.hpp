#pragma once

#include "solver_core.hpp"
#include "items.hpp"

namespace ratio
{
  class flaw;
  class resolver;
  class atom_flaw;
  class enum_flaw;
  class solver;

  class enum_item : public riddle::enum_item
  {
  public:
    enum_item(riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev, enum_flaw &flw) noexcept : riddle::enum_item(tp, std::move(values), ev), flw(flw) {}

    [[nodiscard]] enum_flaw &get_flaw() noexcept { return flw; }

    riddle::expr get(std::string_view name) override;

  private:
    enum_flaw &flw; // the flaw associated with this enum..
  };

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
    node(std::optional<std::reference_wrapper<node>> parent = std::nullopt) noexcept;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] double get_estimated_cost() const noexcept;

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    std::optional<std::reference_wrapper<node>> parent;     // The parent node..
    bool consistent = true;                                 // Whether the node is consistent..
    std::vector<std::shared_ptr<resolver>> resolvers;       // The resolvers applied within this node..
    std::unordered_set<std::shared_ptr<flaw>> open_flaws;   // The set of open flaws..
    std::unordered_set<std::shared_ptr<flaw>> closed_flaws; // The set of closed flaws..
  };

  class solver : public solver_core
  {
    friend class flaw;
    friend class resolver;
    friend class enum_item;
    static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  public:
    solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;

    void solve() override;

    [[nodiscard]] json::json to_json() const override;

  private:
    /**
     * @brief Creates a new flaw of the given type.
     *
     * @tparam Tp The type of the flaw to create.
     * @tparam Args The types of the arguments to pass to the flaw
     * @param args The arguments to pass to the flaw
     * @return Tp& The created flaw
     */
    template <typename Tp, typename... Args>
    Tp &new_flaw(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<flaw, Tp>, "Tp must be a subclass of flaw");
      auto f = std::make_shared<Tp>(std::forward<Args>(args)...);
      auto &f_ref = *f;
      c_node->get().open_flaws.insert(f);
#ifdef ORATIO_ENABLE_LISTENERS
      flaw_created(c_node->get(), f_ref);
#endif
      return f_ref;
    }

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
     * @brief This function is called when a resolver is applied on a node.
     *
     * This function should be overridden by derived classes to handle the event of a resolver application.
     *
     * @param n The node on which the resolver was applied.
     * @param r The applied resolver.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void resolver_applied([[maybe_unused]] const node &n, [[maybe_unused]] const resolver &r) noexcept {}
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
    /**
     * @brief This function is called when a node is found to be inconsistent.
     *
     * This function should be overridden by derived classes to handle the event of an inconsistent node.
     *
     * @param n The inconsistent node.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void inconsistent_node([[maybe_unused]] const node &n) noexcept {}
#endif

  private:
    std::vector<std::unique_ptr<node>> nodes;           // All nodes created during the solving process..
    std::optional<std::reference_wrapper<node>> c_node; // The current node in the search tree..
    std::vector<std::reference_wrapper<node>> fringe;   // The fringe of the search tree..
  };
} // namespace ratio
