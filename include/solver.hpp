#pragma once

#include "core.hpp"
#include "linspire.hpp"

namespace ratio
{
  class solver : public riddle::core
  {
  public:
    solver(std::string_view name = "oRatio") noexcept;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_term &expr) const noexcept override;

  private:
    std::vector<utils::lbool> assigns; // for each variable, the current assignment..
    linspire::solver lin_slv;          // the linear solver..
  };
} // namespace ratio
