#pragma once

#include "flaw.hpp"
#include "resolver.hpp"
#include "type.hpp"

namespace ratio
{
  class atom;
  class smart_type;

  class atom_flaw final : public flaw
  {
    friend class smart_type;

  public:
    atom_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, bool is_fact, riddle::predicate &pred, std::map<std::string, std::shared_ptr<riddle::item>> &&arguments) noexcept;

    [[nodiscard]] const std::shared_ptr<atom> &get_atom() const noexcept { return atm; }

  private:
    void compute_resolvers() override;

    class activate_fact final : public resolver
    {
    public:
      activate_fact(atom_flaw &af);
      activate_fact(atom_flaw &af, const utils::lit &rho);

      void apply() override;

#ifdef ENABLE_VISUALIZATION
      json::json get_data() const noexcept override;
#endif
    };

    class activate_goal final : public resolver
    {
    public:
      activate_goal(atom_flaw &af);
      activate_goal(atom_flaw &af, const utils::lit &rho);

      void apply() override;

#ifdef ENABLE_VISUALIZATION
      json::json get_data() const noexcept override;
#endif
    };

    class unify_atom final : public resolver
    {
    public:
      unify_atom(atom_flaw &af, std::shared_ptr<atom> target, const utils::lit &unify_lit);

      void apply() override;

#ifdef ENABLE_VISUALIZATION
      json::json get_data() const noexcept override;
#endif

    private:
      std::shared_ptr<atom> target;
      utils::lit unify_lit;
    };

#ifdef ENABLE_VISUALIZATION
    json::json get_data() const noexcept override;
#endif

  private:
    std::shared_ptr<atom> atm;
  };
} // namespace ratio
