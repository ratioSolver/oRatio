#pragma once

#include "items.hpp"
#include <vector>
#include <functional>

namespace ratio
{
  class solver;
  class resolver;

  class flaw
  {
  public:
    flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, const bool &exclusive = false) noexcept;
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

  private:
    solver &slv;
    std::vector<std::reference_wrapper<resolver>> causes;
    const bool exclusive;
  };

  class resolver
  {
  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    std::shared_ptr<riddle::enum_item> var;
  };

  class atom_flaw final : public flaw
  {
  public:
    atom_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma) noexcept;

    [[nodiscard]] const riddle::atom_expr &get_atom() const noexcept { return atm; }

  private:
    riddle::atom_expr atm;
  };
} // namespace ratio
