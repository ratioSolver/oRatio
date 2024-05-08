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
  const json::json agent_schema{{"agent", {{"type", "object"}, {"properties", {{"id", {{"type", "integer"}}}, {"type", {{"type", "string"}, {"enum", {"Agent"}}}, {"name", {{"type", "string"}}}, {"values", {{"type", "array"}, {"items", "integer"}}}}}}, {"required", {"id", "type", "name"}}}}};
#endif
} // namespace ratio
