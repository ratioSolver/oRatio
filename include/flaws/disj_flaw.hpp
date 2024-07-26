#pragma once

#include "flaw.hpp"
#include "resolver.hpp"

namespace ratio
{
  class disj_flaw final : public flaw
  {
  public:
    disj_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<utils::lit> &&lits, bool exclusive = false) noexcept;

    [[nodiscard]] const std::vector<utils::lit> &get_lits() const noexcept { return lits; }

  private:
    void compute_resolvers() override;

    class choose_lit final : public resolver
    {
    public:
      choose_lit(disj_flaw &ef, const utils::rational &cost, const utils::lit &l);

      void apply() override {}

#ifdef ENABLE_API
      json::json get_data() const noexcept override;
#endif
    };

#ifdef ENABLE_API
    json::json get_data() const noexcept override;
#endif

  private:
    std::vector<utils::lit> lits;
  };
} // namespace ratio
