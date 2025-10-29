#pragma once

#include "items.hpp"
#include "conjunction.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"
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
    friend class solver;

  public:
    resolver(flaw &f, utils::rational &&intrinsic_cost) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

  private:
    flaw &f;                                                            // the flaw solved by this resolver..
    utils::rational intrinsic_cost;                                     // the intrinsic cost of this resolver..
    std::shared_ptr<linspire::constraint> cnst;                         // the constraint associated with this resolver..
    std::vector<std::shared_ptr<arc_consistency::constraint>> ac_cnsts; // the arc consistency constraints associated with this resolver..
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> var) noexcept;

    [[nodiscard]] const std::shared_ptr<riddle::enum_item> &get_var() const noexcept { return var; }

  private:
    std::shared_ptr<riddle::enum_item> var;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive = false) noexcept;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
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
