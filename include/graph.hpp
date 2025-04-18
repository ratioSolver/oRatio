#pragma once

#include "core.hpp"
#include <deque>
#include <unordered_set>
#include <unordered_map>

#ifdef ENABLE_API
#define STATE_CHANGED() state_changed()
#define NEW_FLAW(f) flaw_created(f)
#define FLAW_STATE_CHANGED(f) flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) flaw_cost_changed(f)
#define FLAW_POSITION_CHANGED(f) flaw_position_changed(f)
#define NEW_RESOLVER(r) resolver_created(r)
#define RESOLVER_STATE_CHANGED(r) resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define NEW_FLAW(f)
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define FLAW_POSITION_CHANGED(f)
#define NEW_RESOLVER(r)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
  class flaw;
  class resolver;

  constexpr const char *origin_kw = "origin";
  constexpr const char *horizon_kw = "horizon";
  constexpr const char *impulse_kw = "Impulse";
  constexpr const char *interval_kw = "Interval";

  class graph : public riddle::core
  {
    friend class flaw;
    friend class resolver;

  public:
    graph(std::string_view name = "oRatio");

    [[nodiscard]] virtual json::json to_json() const override;

  protected:
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
      auto f = utils::make_u_ptr<Tp>(std::forward<Args>(args)...);
      auto &f_ref = *f;
      NEW_FLAW(f_ref);
      flaws.emplace_back(std::move(f));
      flaw_q.push_back(f_ref); // add to the flaw queue..
      if (f_ref.get_causes().empty())
        active_flaws.emplace(&f_ref); // add to the active flaws..
      return f_ref;
    }

    /**
     * @brief Creates a new resolver of the given type.
     *
     * @tparam Tp The type of the resolver to create.
     * @tparam Args The types of the arguments to pass to the resolver
     * @param args The arguments to pass to the resolver
     * @return Tp& The created resolver
     */
    template <typename Tp, typename... Args>
    Tp &new_resolver(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<resolver, Tp>, "Tp must be a subclass of resolver");
      auto r = utils::make_u_ptr<Tp>(std::forward<Args>(args)...);
      auto &r_ref = *r;
      NEW_RESOLVER(r_ref);
      resolvers.emplace_back(std::move(r));
      return r_ref;
    }

    [[nodiscard]] std::vector<utils::ref_wrapper<flaw>> get_flaws() const noexcept;
    [[nodiscard]] std::vector<utils::ref_wrapper<resolver>> get_resolvers() const noexcept;

    [[nodiscard]] std::optional<utils::ref_wrapper<flaw>> get_current_flaw() const noexcept { return c_flaw; }
    void set_current_flaw(std::optional<utils::ref_wrapper<flaw>> flaw) noexcept
    {
      c_flaw = flaw;
      CURRENT_FLAW(flaw);
    }

    [[nodiscard]] std::optional<utils::ref_wrapper<resolver>> get_current_resolver() const noexcept { return c_res; }
    void set_current_resolver(std::optional<utils::ref_wrapper<resolver>> resolver) noexcept
    {
      c_res = resolver;
      CURRENT_RESOLVER(resolver);
    }

    void set_flaw_state(flaw &f, utils::lbool state, bool resetting = false) noexcept;
    void set_flaw_position(flaw &f, size_t pos) noexcept;
    void set_resolver_state(resolver &r, utils::lbool state, bool resetting = false) noexcept;

    [[nodiscard]] std::vector<utils::ref_wrapper<flaw>> get_queued_flaws() const noexcept;

    [[nodiscard]] const std::unordered_set<flaw *> &get_active_flaws() const noexcept { return active_flaws; }

    /**
     * @brief Builds the graph.
     *
     * This function builds the graph by expanding the flaws and applying the resolvers.
     */
    void build();

    /**
     * @brief Adds a layer to the graph.
     *
     * This function adds a layer to the graph by expanding the fringe flaws and applying the resolvers.
     */
    void add_layer();

    void add_causal_link(flaw &f, resolver &r) noexcept;

    void push() noexcept;

    void pop() noexcept;

  protected:
    void expand_flaw(flaw &f);

  private:
    void compute_flaw_cost(flaw &f);

    virtual void added_causal_link(flaw &, resolver &) {}

    bool is_deferrable(flaw &f); // checks whether the given flaw is deferrable..

#ifdef BUILD_LISTENERS
  protected:
    /**
     * @brief This function is called when the state of the solver changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() {}

  private:
    /**
     * @brief Notifies when a flaw has been created.
     *
     * This function is called when a flaw has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw is created.
     *
     * @param flaw The flaw that has been created.
     */
    virtual void flaw_created(const flaw &) {}
    /**
     * @brief Notifies when the state of a flaw has changed.
     *
     * This function is called when the state of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's state changes.
     *
     * @param flaw The flaw whose state has changed.
     */
    virtual void flaw_state_changed(const flaw &) {}
    /**
     * @brief Notifies when the cost of a flaw has changed.
     *
     * This function is called when the cost of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's cost changes.
     *
     * @param flaw The flaw whose cost has changed.
     */
    virtual void flaw_cost_changed(const flaw &) {}
    /**
     * @brief Notifies when the position of a flaw has changed.
     *
     * This function is called when the position of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's position changes.
     *
     * @param flaw The flaw whose position has changed.
     */
    virtual void flaw_position_changed(const flaw &) {}
    /**
     * @brief Notifies when the current flaw has changed.
     *
     * This function is called when the current flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current flaw changes.
     *
     * @param flaw The current flaw.
     */
    virtual void current_flaw(std::optional<utils::ref_wrapper<flaw>>) {}

    /**
     * @brief Notifies when a resolver has been created.
     *
     * This function is called when a resolver has been created. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver is created.
     *
     * @param resolver The resolver that has been created.
     */
    virtual void resolver_created(const resolver &) {}
    /**
     * @brief Notifies when the state of a resolver has changed.
     *
     * This function is called when the state of a resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a resolver's state changes.
     *
     * @param resolver The resolver whose state has changed.
     */
    virtual void resolver_state_changed(const resolver &) {}
    /**
     * @brief Notifies when the current resolver has changed.
     *
     * This function is called when the current resolver has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when the current resolver changes.
     *
     * @param resolver The current resolver.
     */
    virtual void current_resolver(std::optional<utils::ref_wrapper<resolver>>) {}

    /**
     * @brief Notifies when a causal link has been added.
     *
     * This function is called when a causal link has been added. It is a virtual function that can be overridden by derived classes to perform specific actions when a causal link is added.
     *
     * @param flaw The flaw that is the source of the causal link.
     * @param resolver The resolver that is the destination of the causal link.
     */
    virtual void causal_link_added(const flaw &, const resolver &) {}
#endif

  private:
    std::vector<utils::u_ptr<flaw>> flaws;             // The set of flaws
    std::vector<utils::u_ptr<resolver>> resolvers;     // The set of resolvers
    std::optional<utils::ref_wrapper<flaw>> c_flaw;    // the current flaw..
    std::optional<utils::ref_wrapper<resolver>> c_res; // the current resolver..
    std::deque<utils::ref_wrapper<flaw>> flaw_q;       // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> active_flaws;           // the currently active flaws..

    struct layer
    {
      std::unordered_map<flaw *, utils::rational> old_f_costs; // the old estimated flaws` costs..
      std::unordered_set<flaw *> new_flaws;                    // the just activated flaws..
      std::unordered_set<flaw *> solved_flaws;                 // the just solved flaws..
    };
    std::vector<layer> trail; // the list of taken decisions, with the associated changes made, in chronological order..
  };

  class flaw
  {
    friend class graph;
    friend class resolver;

  public:
    flaw(graph &gr, std::vector<utils::ref_wrapper<resolver>> &&causes, const bool &exclusive = false);
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] graph &get_graph() noexcept { return gr; }
    [[nodiscard]] const graph &get_graph() const noexcept { return gr; }

    [[nodiscard]] const std::vector<utils::ref_wrapper<resolver>> &get_causes() const noexcept { return causes; }

    [[nodiscard]] bool is_exclusive() const noexcept { return exclusive; }

    [[nodiscard]] const std::vector<utils::ref_wrapper<resolver>> &get_resolvers() const noexcept { return resolvers; }

    [[nodiscard]] const utils::rational &get_estimated_cost() const noexcept { return est_cost; }

    [[nodiscard]] const std::vector<utils::ref_wrapper<resolver>> &get_supports() const noexcept { return supports; }

    [[nodiscard]] virtual json::json to_json() const;

    [[nodiscard]] bool is_expanded() const noexcept { return expanded; }

    [[nodiscard]] utils::lbool get_state() const noexcept { return state; }

    [[nodiscard]] size_t get_position() const noexcept { return position; }

  protected:
    void set_state(utils::lbool state) noexcept;

    template <typename Tp, typename... Args>
    Tp &new_resolver(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<resolver, Tp>, "Tp must be a subclass of resolver");
      return gr.new_resolver<Tp>(std::forward<Args>(args)...);
    }

  private:
    virtual void compute_resolvers() = 0;
    virtual void expanded_flaw() {}

  private:
    graph &gr;                                                     // the graph this flaw belongs to..
    std::vector<utils::ref_wrapper<resolver>> causes;              // the causes of this flaw..
    const bool exclusive;                                          // whether this flaw is exclusive or not..
    std::vector<utils::ref_wrapper<resolver>> resolvers;           // the resolvers for this flaw..
    utils::rational est_cost = utils::rational::positive_infinite; // the current estimated cost of the flaw..
    std::vector<utils::ref_wrapper<resolver>> supports;            // the resolvers supported by this flaw (used for propagating cost estimates)..
    bool expanded = false;                                         // whether this flaw has been expanded or not..
    utils::lbool state = utils::Undefined;                         // the current state of the flaw..
    size_t position = 0;                                           // the position of the flaw in the graph..
  };

  class resolver
  {
    friend class graph;
    friend class flaw;

  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost);
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] flaw &get_flaw() noexcept { return f; }
    [[nodiscard]] const flaw &get_flaw() const noexcept { return f; }

    [[nodiscard]] utils::lbool get_state() const noexcept { return state; }

    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

    [[nodiscard]] const std::vector<utils::ref_wrapper<flaw>> &get_preconditions() const noexcept { return preconditions; }

    [[nodiscard]] utils::rational get_estimated_cost() const noexcept;

    [[nodiscard]] virtual json::json to_json() const;

  protected:
    void set_state(utils::lbool state) noexcept;

  private:
    virtual void apply() = 0;

  private:
    flaw &f;                                             // the flaw solved by this resolver..
    utils::lbool state = utils::Undefined;               // the current state of the resolver..
    utils::rational intrinsic_cost;                      // the intrinsic cost of this resolver..
    std::vector<utils::ref_wrapper<flaw>> preconditions; // the preconditions of this resolver..
  };

  inline std::string to_string(const utils::lbool &node_state) noexcept
  {
    switch (node_state)
    {
    case utils::True:
      return "active";
    case utils::False:
      return "forbidden";
    default:
      return "inactive";
    }
  }
} // namespace ratio
