#pragma once

#include "semitone_api.hpp"

namespace utils
{
  class rational;
  class inf_rational;
} // namespace utils

namespace riddle
{
  class item;
} // namespace riddle

namespace ratio
{
  class solver;
  class graph;
  class flaw;
  class resolver;

  /**
   * @brief Get the JSON representation of the given rational.
   *
   * @param rat the rational to get the JSON representation of.
   * @return json::json the JSON representation of the given rational.
   */
  [[nodiscard]] json::json to_json(const utils::rational &rat) noexcept;

  /**
   * @brief Get the JSON representation of the given infinitesimal rational.
   *
   * @param rat the infinitesimal rational to get the JSON representation of.
   * @return json::json the JSON representation of the given infinitesimal rational.
   */
  [[nodiscard]] json::json to_json(const utils::inf_rational &rat) noexcept;

  /**
   * @brief Get the JSON representation of the given item.
   *
   * @param rhs the item to get the JSON representation of.
   * @return json::json the JSON representation of the given item.
   */
  [[nodiscard]] json::json to_json(const riddle::item &rhs) noexcept;

  /**
   * @brief Get the JSON representation of the value of the given item.
   *
   * @param itm the item to get the JSON representation of.
   * @return json::json the JSON representation of the value of the given item.
   */
  [[nodiscard]] json::json value(const riddle::item &itm) noexcept;

  /**
   * @brief Get the JSON representation of the given solver.
   *
   * @param rhs the solver to get the JSON representation of.
   * @return json::json the JSON representation of the given solver.
   */
  [[nodiscard]] json::json to_json(const solver &rhs) noexcept;
  /**
   * @brief Get the JSON representation of the timelines of the given solver.
   *
   * @param rhs the solver to get the timelines of.
   * @return json::json the JSON representation of the timelines of the given solver.
   */
  [[nodiscard]] json::json to_timelines(const solver &rhs) noexcept;

  /**
   * @brief Converts a graph object to a JSON representation.
   *
   * This function takes a graph object as input and converts it to a JSON representation.
   * The resulting JSON object can be used for various purposes such as serialization or data exchange.
   *
   * @param rhs The graph object to be converted to JSON.
   * @return A JSON object representing the graph.
   */
  [[nodiscard]] json::json to_json(const graph &rhs) noexcept;

  /**
   * @brief Converts a flaw object to a JSON representation.
   *
   * This function takes a flaw object as input and converts it to a JSON object.
   * The resulting JSON object represents the flaw in a serialized format.
   *
   * @param f The flaw object to be converted to JSON.
   * @return A JSON object representing the flaw.
   */
  [[nodiscard]] json::json to_json(const flaw &f) noexcept;

  /**
   * Converts a flaw object to its corresponding state as a string.
   *
   * @param f The flaw object to convert.
   * @return The state as a string.
   */
  [[nodiscard]] std::string to_state(const flaw &f) noexcept;

  /**
   * @brief Converts a resolver object to a JSON representation.
   *
   * This function takes a resolver object and converts it to a JSON object using the json::json class.
   * The resulting JSON object represents the state of the resolver.
   *
   * @param r The resolver object to convert to JSON.
   * @return A JSON object representing the state of the resolver.
   */
  [[nodiscard]] json::json to_json(const resolver &r) noexcept;

  /**
   * Converts a resolver object to a string representation of its state.
   *
   * @param r The resolver object to convert.
   * @return A string representation of the resolver's state.
   */
  [[nodiscard]] std::string to_state(const resolver &r) noexcept;

  /**
   * @brief Creates a JSON message representing a causal graph.
   *
   * This function takes a graph object and converts it into a JSON message.
   * The resulting JSON message includes the graph data as well as a type field
   * indicating that it represents a graph.
   *
   * @param g The graph object to be converted.
   * @return A JSON message representing the graph.
   */
  [[nodiscard]] json::json make_solver_graph_message(const graph &g) noexcept;

  /**
   * Creates a JSON message indicating the creation of a new flaw.
   *
   * @param f The flaw object.
   * @return A JSON object representing the flaw creation message.
   */
  [[nodiscard]] json::json make_flaw_created_message(const flaw &f) noexcept;

  /**
   * @brief Creates a JSON message indicating a change in flaw state.
   *
   * @param f The flaw for which the state change message is being created.
   * @return A JSON object representing the flaw state change message.
   */
  [[nodiscard]] json::json make_flaw_state_changed_message(const flaw &f) noexcept;

  /**
   * @brief Creates a JSON message indicating a change in flaw cost.
   *
   * @param f The flaw for which the cost change message is being created.
   * @return A JSON object representing the flaw cost change message.
   */
  [[nodiscard]] json::json make_flaw_cost_changed_message(const flaw &f) noexcept;

  /**
   * @brief Creates a JSON message indicating a change in flaw position.
   *
   * @param f The flaw for which the position change message is being created.
   * @return A JSON object representing the flaw position change message.
   */
  [[nodiscard]] json::json make_flaw_position_changed_message(const flaw &f) noexcept;

  /**
   * @brief Creates a JSON message indicating the current flaw.
   *
   * @param f The current flaw.
   * @return A JSON object representing the current flaw message.
   */
  [[nodiscard]] json::json make_current_flaw_message(const flaw &f) noexcept;

  /**
   * @brief Creates a JSON message indicating the creation of a new resolver.
   *
   * @param r The resolver object.
   * @return A JSON object representing the resolver creation message.
   */
  [[nodiscard]] json::json make_resolver_created_message(const resolver &r) noexcept;

  /**
   * @brief Creates a JSON message indicating a change in resolver state.
   *
   * @param r The resolver for which the state change message is being created.
   * @return A JSON object representing the resolver state change message.
   */
  [[nodiscard]] json::json make_resolver_state_changed_message(const resolver &r) noexcept;

  /**
   * @brief Creates a JSON message indicating the current resolver.
   *
   * @param r The current resolver.
   * @return A JSON object representing the current resolver message.
   */
  [[nodiscard]] json::json make_current_resolver_message(const resolver &r) noexcept;

  /**
   * @brief Creates a JSON message indicating the addition of a causal link between a flaw and a resolver.
   *
   * @param f The flaw for which the causal link was added.
   * @param r The resolver for which the causal link was added.
   * @return A JSON object representing the causal link added message.
   */
  [[nodiscard]] json::json make_causal_link_added_message(const flaw &f, const resolver &r) noexcept;

  const json::json solver_schemas{
      {"rational",
       {{"type", "object"},
        {"properties",
         {{"num", {{"type", "integer"}}},
          {"den", {{"type", "integer"}}}}},
        {{"required", std::vector<json::json>{"num", "den"}}}}},
      {"inf_rational",
       {{"type", "object"},
        {"properties",
         {{"num", {{"type", "integer"}}},
          {"den", {{"type", "integer"}}},
          {"inf", {{"$ref", "#/components/schemas/rational"}}}}},
        {{"required", std::vector<json::json>{"num", "den"}}}}},
      {"value",
       {{"oneOf", std::vector<json::json>{{"$ref", "#/components/schemas/bool_value"}, {"$ref", "#/components/schemas/int_value"}, {"$ref", "#/components/schemas/real_value"}, {"$ref", "#/components/schemas/time_value"}, {"$ref", "#/components/schemas/string_value"}, {"$ref", "#/components/schemas/enum_value"}, {"$ref", "#/components/schemas/item_value"}}}}},
      {"bool_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"bool"}}}},
          {"lit", {{"type", "string"}}},
          {"val", {{"type", "string"}, {"enum", {"True", "False", "Undefined"}}}}}},
        {"required", {"type", "lit", "val"}}}},
      {"int_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"int"}}}},
          {"lin", {{"type", "string"}}},
          {"val", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"lb", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"ub", {{"$ref", "#/components/schemas/inf_rational"}}}}},
        {"required", {"type", "lin", "val"}}}},
      {"real_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"real"}}}},
          {"lin", {{"type", "string"}}},
          {"val", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"lb", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"ub", {{"$ref", "#/components/schemas/inf_rational"}}}}},
        {"required", {"type", "lin", "val"}}}},
      {"time_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"time"}}}},
          {"lin", {{"type", "string"}}},
          {"val", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"lb", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"ub", {{"$ref", "#/components/schemas/inf_rational"}}}}},
        {"required", {"type", "lin", "val"}}}},
      {"string_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"string"}}}},
          {"val", {{"type", "string"}}}}},
        {"required", std::vector<json::json>{"type", "val"}}}},
      {"enum_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"enum"}}}},
          {"var", {{"type", "string"}}},
          {"vals", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
        {"required", {"type", "var", "vals"}}}},
      {"item_value",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"item"}}}},
          {"val", {{"type", "integer"}}}}},
        {"required", std::vector<json::json>{"type", "val"}}}},
      {"item",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"type", {{"type", "string"}}},
          {"name", {{"type", "string"}}},
          {"exprs", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/value"}}}}}}},
        {"required", {"id", "type", "name"}}}},
      {"atom",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"is_fact", {{"type", "boolean"}}},
          {"sigma", {{"type", "integer"}}},
          {"type", {{"type", "string"}}},
          {"status", {{"type", "string"}, {"enum", {"Active", "Inactive", "Unified"}}}},
          {"name", {{"type", "string"}}},
          {"exprs", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/value"}}}}}}},
        {"required", {"id", "is_fact", "sigma", "type", "name", "status"}}}},
      {"solver_state",
       {{"type", "object"},
        {"properties",
         {{"name", {{"type", "string"}}},
          {"items", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/item"}}}}},
          {"atoms", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/atom"}}}}},
          {"exprs", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/value"}}}}}}},
        {"required", {"name"}}}},
      {"flaw",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"causes", {{"type", "array"}, {"description", "The resolvers that caused this flaw."}, {"items", {{"type", "integer"}}}}},
          {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}},
          {"phi", {{"type", "string"}}},
          {"cost", {{"$ref", "#/components/schemas/rational"}}},
          {"pos", {{"type", "integer"}}},
          {"data", {{"type", "object"}}}}},
        {"required", {"id", "causes", "state", "pos", "phi", "cost"}}}},
      {"resolver",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}},
          {"flaw", {{"type", "integer"}, {"description", "The flaw this resolver solves."}}},
          {"preconditions", {{"type", "array"}, {"description", "The preconditions that must be satisfied for this resolver to be applied."}, {"items", {{"type", "integer"}}}}},
          {"rho", {{"type", "string"}}},
          {"intrinsic_cost", {{"$ref", "#/components/schemas/rational"}}},
          {"pos", {{"type", "integer"}}},
          {"data", {{"type", "object"}}}}},
        {"required", {"id", "state", "flaw", "rho", "pos", "preconditions", "intrinsic_cost"}}}},
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
        {"required", std::vector<json::json>{"flaws", "resolvers"}}}},
      {"timeline",
       {{"oneOf", std::vector<json::json>{{"$ref", "#/components/schemas/solver_timeline"}, {"$ref", "#/components/schemas/agent_timeline"}, {"$ref", "#/components/schemas/state_variable_timeline"}, {"$ref", "#/components/schemas/reusable_resource_timeline"}, {"$ref", "#/components/schemas/consumable_resource_timeline"}}}}},
      {"solver_timeline",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"type", {{"type", "string"}, {"enum", {"Solver"}}}},
          {"name", {{"type", "string"}}},
          {"values", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
        {"required", {"id", "type", "name"}}}},
      {"agent_timeline",
       {{"type", "object"},
        {"description", "Schema representing an agent timeline"},
        {"properties",
         {{"id", {{"type", "integer"}, {"description", "The ID of the agent"}}},
          {"type", {{"type", "string"}, {"enum", {"Agent"}}}},
          {"name", {{"type", "string"}, {"description", "The name of the agent"}}},
          {"values", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Array of atom IDs"}}}}},
        {"required", {"id", "type", "name"}}}},
      {"state_variable_timeline_value",
       {{"type", "object"},
        {"properties",
         {{"from", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"to", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"atoms", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
        {"required", {"from", "to", "atoms"}}}},
      {"state_variable_timeline",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"type", {{"type", "string"}, {"enum", {"StateVariable"}}}},
          {"name", {{"type", "string"}}},
          {"values", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/state_variable_timeline_value"}}}}}}},
        {"required", {"id", "type", "name"}}}},
      {"reusable_resource_timeline_value",
       {{"type", "object"},
        {"properties",
         {{"from", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"to", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"usage", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"atoms", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
        {"required", {"from", "to", "usage"}}}},
      {"reusable_resource_timeline",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"type", {{"type", "string"}, {"enum", {"ReusableResource"}}}},
          {"name", {{"type", "string"}}},
          {"capacity", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"values", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/reusable_resource_timeline_value"}}}}}}},
        {"required", {"id", "type", "name", "capacity"}}}},
      {"consumable_resource_timeline_value",
       {{"type", "object"},
        {"properties",
         {{"from", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"to", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"start", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"end", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"atoms", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
        {"required", {"from", "to", "start", "end"}}}},
      {"consumable_resource_timeline",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"type", {{"type", "string"}, {"enum", {"ConsumableResource"}}}},
          {"name", {{"type", "string"}}},
          {"capacity", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"initial_amount", {{"$ref", "#/components/schemas/inf_rational"}}},
          {"values", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/consumable_resource_timeline_value"}}}}}}},
        {"required", {"id", "type", "name", "capacity", "initial_amount"}}}}};

  const json::json solver_messages{
      {"flaw_created_message",
       {"payload",
        {{"allOf",
          std::vector<json::json>{{"$ref", "#/components/schemas/flaw"}}},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_created"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}}}}}}},
      {"flaw_state_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_state_changed"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the flaw"}}},
           {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}}}}}}},
      {"flaw_cost_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_cost_changed"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the flaw"}}},
           {"cost", {{"$ref", "#/components/schemas/rational"}}}}}}}},
      {"flaw_position_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"flaw_position_changed"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the flaw"}}},
           {"position", {{"type", "integer"}}}}}}}},
      {"current_flaw_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"current_flaw"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the flaw"}}}}}}}},
      {"resolver_created_message",
       {"payload",
        {{"allOf",
          std::vector<json::json>{{"$ref", "#/components/schemas/resolver"}}},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"resolver_created"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}}}}}}},
      {"resolver_state_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"resolver_state_changed"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the resolver"}}},
           {"state", {{"type", "string"}, {"enum", {"active", "forbidden", "inactive"}}}}}}}}},
      {"current_resolver_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"current_resolver"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the resolver"}}}}}}}},
      {"causal_link_added_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"causal_link_added"}}}},
           {"solver_id", {{"type", "integer"}, {"description", "The ID of the solver"}}},
           {"flaw_id", {{"type", "integer"}, {"description", "The ID of the flaw"}}},
           {"resolver_id", {{"type", "integer"}, {"description", "The ID of the resolver"}}}}}}}},
      {"graph_message",
       {"payload",
        {{"allOf", std::vector<json::json>{{"$ref", "#/components/schemas/solver_graph"}}},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solver_graph"}}}},
           {"id", {{"type", "integer"}, {"description", "The ID of the solver"}}}}}}}}};
} // namespace ratio
