#pragma once
#include "h_1.h"

namespace ratio
{
  class meta_flaw : public flaw
  {
  public:
    meta_flaw(flaw &f, std::vector<std::reference_wrapper<resolver>> cs, std::unordered_set<resolver *> ms);

    flaw &get_flaw() const noexcept { return f; }

  private:
    void compute_resolvers() override;

    json::json get_data() const noexcept override { return {{"type", "meta-flaw"}, {"flaw", get_id(f)}}; }

  private:
    flaw &f;
    std::unordered_set<resolver *> mutexes;
  };

  class meta_resolver : public resolver
  {
  public:
    meta_resolver(meta_flaw &mf, const resolver &res);

    const resolver &get_resolver() const noexcept { return res; }

  private:
    void apply() override {}

    json::json get_data() const noexcept override { return {{"type", "meta-resolver"}, {"resolver", get_id(res)}}; }

  private:
    meta_flaw &mf;
    const resolver &res;
  };

  class h_2 final : public h_1
  {
  public:
    h_2(solver &s);
  };
} // namespace ratio
