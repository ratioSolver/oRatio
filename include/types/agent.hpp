#pragma once

#include "smart_type.hpp"

namespace ratio
{
  class agent final : public smart_type, public timeline
  {
  public:
    agent(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() noexcept override { return {}; }

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_VISUALIZATION
    json::json extract() const noexcept override;
#endif

  private:
    std::vector<std::reference_wrapper<atom>> atoms;
  };

#ifdef ENABLE_VISUALIZATION
  const json::json agent_timeline_schema{{"agent_timeline", {{"type", "object"}, {"description", "Schema representing an agent timeline"}, {"properties", {{"id", {{"type", "integer"}, {"description", "The ID of the agent"}}}, {"type", {{"type", "string"}, {"enum", {"Agent"}}}}, {"name", {{"type", "string"}, {"description", "The name of the agent"}}}, {"values", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Array of atom IDs"}}}}}, {"required", {"id", "type", "name"}}}}};
#endif
} // namespace ratio
