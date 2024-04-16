#pragma once

#include "smart_type.hpp"

namespace ratio
{
  class state_variable final : public smart_type, public timeline
  {
  public:
    state_variable(solver &slv);

  private:
    std::vector<std::vector<std::pair<utils::lit, double>>> get_current_incs() const noexcept override { return {}; }

    void new_atom(std::shared_ptr<ratio::atom> &atm) noexcept override;

#ifdef ENABLE_VISUALIZATION
    json::json extract() const noexcept override;
#endif

  private:
    std::vector<std::reference_wrapper<atom>> atoms;
  };
} // namespace ratio
