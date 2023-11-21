#pragma once

#include "h_1.h"

namespace ratio
{
  class h_2 : public h_1
  {
  public:
    h_2(solver &s);

#ifdef GRAPH_REFINING
    void refine() override;

    void visit(flaw &f);

    void negated_resolver(resolver &r) override;

    class h_2_flaw : public flaw
    {
    public:
      h_2_flaw(flaw &sub_f, resolver &r, resolver &mtx_r);

    private:
      void compute_resolvers() override;

      json::json get_data() const noexcept override;

      class h_2_resolver : public resolver
      {
      public:
        h_2_resolver(h_2_flaw &f, resolver &sub_r);

        void apply() override;

        json::json get_data() const noexcept override;

      private:
        resolver &sub_r;
      };

    private:
      flaw &sub_f;
      resolver &mtx_r;
    };

  private:
    resolver *c_res = nullptr;
    std::vector<flaw *> h_2_flaws;
#endif
  };
} // namespace ratio