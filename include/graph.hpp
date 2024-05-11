#pragma once

#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include "solver.hpp"
#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class smart_type;
  class graph : public semitone::theory
  {
    friend class smart_type;

  public:
    graph(solver &slv) noexcept;

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
      auto f = new Tp(std::forward<Args>(args)...);
      if (slv.get_sat().root_level())
      { // if we are at root-level, we initialize the flaw and add it to the flaw queue for future expansion..
        f->init();
        phis[variable(f->get_phi())].push_back(std::unique_ptr<flaw>(f));
        flaw_q.push_back(*f); // we add the flaw to the flaw queue..
      }
      else // we add the flaw to the pending flaws..
        pending_flaws.push_back(std::unique_ptr<flaw>(f));
      return *f;
    }

    /**
     * Returns a vector of flaws in the graph.
     *
     * @return A vector of reference wrappers to the flaws.
     */
    std::vector<std::reference_wrapper<flaw>> get_flaws() const noexcept
    {
      std::vector<std::reference_wrapper<flaw>> res;
      for (auto &f : phis)
        for (auto &f_ : f.second)
          res.push_back(*f_);
      return res;
    }

    /**
     * @brief Get the current flaw.
     *
     * This function returns an optional reference to the current flaw.
     *
     * @return An optional reference to the current flaw.
     */
    const std::optional<std::reference_wrapper<flaw>> &get_current_flaw() const noexcept { return c_flaw; }

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
      auto r = new Tp(std::forward<Args>(args)...);
      rhos[variable(r->get_rho())].push_back(std::unique_ptr<resolver>(r));
      r->get_flaw().resolvers.push_back(*r);
      return *r;
    }

    /**
     * Returns a vector of resolvers in the graph.
     *
     * @return A vector of reference wrappers to the resolvers.
     */
    std::vector<std::reference_wrapper<resolver>> get_resolvers() const noexcept
    {
      std::vector<std::reference_wrapper<resolver>> res;
      for (auto &r : rhos)
        for (auto &r_ : r.second)
          res.push_back(*r_);
      return res;
    }

    /**
     * @brief Get the current resolver.
     *
     * This function returns an optional reference to the current resolver.
     *
     * @return An optional reference to the current resolver.
     */
    const std::optional<std::reference_wrapper<resolver>> &get_current_resolver() const noexcept { return c_res; }

    /**
     * @brief Creates a new causal link between a flaw and a resolver.
     *
     * This function establishes a causal link between the given flaw and resolver.
     * A causal link represents a dependency relationship between a flaw and a resolver,
     * indicating that the flaw must be active for the resolver to be applicable.
     *
     * @param f The flaw to establish a causal link with.
     * @param r The resolver to establish a causal link with.
     */
    void new_causal_link(flaw &f, resolver &r);

    /**
     * @brief Gets the current controlling literal.
     *
     * If there is a current resolver, the controlling literal is the rho variable of the resolver. Otherwise, it is TRUE.
     *
     * @return const utils::lit& The current controlling literal.
     */
    const utils::lit &get_ni() const noexcept { return ni; }

    /**
     * @brief Builds the graph.
     *
     * This function is responsible for building the graph.
     */
    void build();

    /**
     * @brief Adds a new layer to the graph.
     *
     * This function is responsible for adding a new layer to the graph.
     */
    void add_layer();

  protected:
    /**
     * @brief Enqueues the given flaw in the graph.
     *
     * @param f The flaw to enqueue.
     */
    void enqueue(flaw &f) noexcept { flaw_q.push_back(f); }

    /**
     * @brief Expands the given flaw in the graph.
     *
     * @param f The flaw to expand.
     */
    void expand_flaw(flaw &f);

    /**
     * @brief Sets the value of `ni`.
     *
     * This function sets the value of `ni` to the specified `v`.
     *
     * @param v The new value for `ni`.
     */
    void set_ni(const utils::lit &v) noexcept
    {
      tmp_ni = ni;
      ni = v;
    }

    /**
     * @brief Restores the value of `ni` to its original value.
     *
     * This function restores the value of the `ni` variable to its original value
     * before any modifications were made.
     */
    void restore_ni() noexcept { ni = tmp_ni; }

  private:
    bool propagate(const utils::lit &) noexcept override { return true; }
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    solver &slv;                                                                    // the solver this graph belongs to..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<flaw>>> phis;     // the phi variables (propositional variable to flaws) of the flaws..
    std::vector<std::unique_ptr<flaw>> pending_flaws;                               // pending flaws, waiting for root-level to be initialized..
    std::unordered_map<VARIABLE_TYPE, std::vector<std::unique_ptr<resolver>>> rhos; // the rho variables (propositional variable to resolver) of the resolvers..
    std::optional<std::reference_wrapper<flaw>> c_flaw;                             // the current flaw..
    std::optional<std::reference_wrapper<resolver>> c_res;                          // the current resolver..
    utils::lit tmp_ni;                                                              // the temporary controlling literal, used for restoring the controlling literal..
    utils::lit ni{utils::TRUE_lit};                                                 // the current controlling literal..
    std::deque<std::reference_wrapper<flaw>> flaw_q;                                // the flaw queue (for the graph building procedure)..
    std::unordered_set<flaw *> active_flaws;                                        // the currently active flaws..
    VARIABLE_TYPE gamma;                                                            // the variable representing the validity of this graph..
  };

#ifdef ENABLE_VISUALIZATION
  json::json to_json(const graph &rhs) noexcept;

  inline json::json graph_message(const graph &g) noexcept
  {
    json::json j = to_json(g);
    j["type"] = "graph";
    return j;
  }

  const json::json solver_graph_schema{
      {"solver_graph",
       {{"type", "object"},
        {"properties",
         {{"flaws",
           {{"type", "array"},
            {"items", {{"$ref", "#/components/schemas/flaw"}}}}},
          {"current_flaw", {{"type", "integer"}}},
          {"resolvers",
           {{"type", "array"},
            {"items", {{"$ref", "#/components/schemas/resolver"}}}}},
          {"current_resolver", {{"type", "integer"}}}}},
        {"required", std::vector<json::json>{"flaws", "resolvers"}}}}};

  const json::json flaw_created_message{
      {"flaw_created_message",
       {"payload",
        {{"$ref", "#/components/schemas/flaw"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_created"}}}},
           {"solver_id", {{"type", "integer"}}}}}}}}};

  const json::json flaw_state_changed_message{
      {"flaw_state_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_state_changed"}}}},
           {"flaw_id", {{"type", "integer"}}},
           {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}}}}}}}};

  const json::json flaw_cost_changed_message{
      {"flaw_cost_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_cost_changed"}}}},
           {"flaw_id", {{"type", "integer"}}},
           {"cost", {{"$ref", "#/components/schemas/rational"}}}}}}}}};

  const json::json flaw_position_changed_message{
      {"flaw_position_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_position_changed"}}}},
           {"flaw_id", {{"type", "integer"}}},
           {"position", {{"type", "integer"}}}}}}}}};

  const json::json current_flaw_message{
      {"current_flaw_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"current_flaw"}}}},
           {"flaw_id", {{"type", "integer"}}}}}}}}};

  const json::json resolver_created_message{
      {"resolver_created_message",
       {"payload",
        {{"$ref", "#/components/schemas/resolver"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"resolver_created"}}}},
           {"solver_id", {{"type", "integer"}}}}}}}}};

  const json::json resolver_state_changed_message{
      {"resolver_state_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"resolver_state_changed"}}}},
           {"resolver_id", {{"type", "integer"}}},
           {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}}}}}}}};

  const json::json current_resolver_message{
      {"current_resolver_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"current_resolver"}}}},
           {"resolver_id", {{"type", "integer"}}}}}}}}};

  const json::json causal_link_added_message{
      {"causal_link_added_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"causal_link_added"}}}},
           {"flaw_id", {{"type", "integer"}}},
           {"resolver_id", {{"type", "integer"}}}}}}}}};
#endif
} // namespace ratio