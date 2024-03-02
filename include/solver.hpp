#pragma once

#include "solver_item.hpp"
#include "core.hpp"
#include "lra_theory.hpp"
#include "idl_theory.hpp"
#include "rdl_theory.hpp"
#include "ov_theory.hpp"
#include "graph.hpp"

namespace ratio
{
  class solver : public riddle::core, public semitone::theory
  {
  public:
    solver(const std::string &name = "oRatio", bool i = true) noexcept;
    virtual ~solver() = default;

    void init() noexcept;

    std::shared_ptr<riddle::item> new_bool() noexcept override;
    std::shared_ptr<riddle::item> new_bool(bool value) noexcept override;

  private:
    std::string name;
    semitone::lra_theory lra;
    semitone::idl_theory idl;
    semitone::rdl_theory rdl;
    semitone::ov_theory ov;
    graph gr;
  };
} // namespace ratio
