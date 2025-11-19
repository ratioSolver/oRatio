#pragma once

#include "solver_core.hpp"

#ifdef ORATIO_ENABLE_LISTENERS
#define NEW_FLAW(f) flaw_created(f)
#define NEW_RESOLVER(r) resolver_created(r)
#else
#define NEW_FLAW(f)
#define NEW_RESOLVER(r)
#endif

namespace ratio
{
  class basic_solver;
  class resolver;

  class flaw
  {
  public:
    flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept;
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

  private:
    virtual void compute_resolvers() = 0;

  protected:
    basic_solver &slv;                                    // The solver managing this flaw..
    std::vector<std::reference_wrapper<resolver>> causes; // The causes of this flaw..
  };

  class resolver
  {
  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

  private:
    virtual void apply() = 0;

  protected:
    flaw &flw;                      // The flaw this resolver addresses..
    utils::rational intrinsic_cost; // The intrinsic cost of applying this resolver..
  };

  class basic_solver : public solver_core
  {
  public:
    basic_solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;

    /**
     * @brief Creates a new flaw of the given type.
     *
     * @tparam Tp The type of the flaw to create.
     * @tparam Args The types of the arguments to pass to the flaw
     * @param args The arguments to pass to the flaw
     * @return Tp& The created flaw
     */
    template <typename Tp, typename... Args>
    Tp &new_flaw(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<flaw, Tp>, "Tp must be a subclass of flaw");
      auto f = std::make_unique<Tp>(std::forward<Args>(args)...);
      auto &f_ref = *f;
      NEW_FLAW(f_ref);
      flaws.emplace_back(std::move(f));
      return f_ref;
    }

    /**
     * @brief Creates a new resolver of the given type.
     *
     * @tparam Tp The type of the resolver to create.
     * @tparam Args The types of the arguments to pass to the resolver
     * @param args The arguments to pass to the resolver
     * @return Tp& The created resolver
     */
    template <typename Tp, typename... Args>
    Tp &new_resolver(Args &&...args) noexcept
    {
      static_assert(std::is_base_of_v<resolver, Tp>, "Tp must be a subclass of resolver");
      auto r = std::make_unique<Tp>(std::forward<Args>(args)...);
      auto &r_ref = *r;
      NEW_RESOLVER(r_ref);
      resolvers.emplace_back(std::move(r));
      return r_ref;
    }

    void solve() override;

#ifdef ORATIO_ENABLE_LISTENERS
  private:
    /**
     * @brief Notifies that a new flaw has been created.
     *
     * This function is called whenever a new flaw is created in the solver.
     *
     * @param f The newly created flaw.
     */
    virtual void flaw_created([[maybe_unused]] flaw &f) noexcept {}

    /**
     * @brief Notifies that a new resolver has been created.
     *
     * This function is called whenever a new resolver is created in the solver.
     *
     * @param r The newly created resolver.
     */
    virtual void resolver_created([[maybe_unused]] resolver &r) noexcept {}
#endif

  private:
    std::vector<std::unique_ptr<flaw>> flaws;              // The set of flaws
    std::vector<std::unique_ptr<resolver>> resolvers;      // The set of resolvers
    std::optional<std::reference_wrapper<flaw>> c_flaw;    // The current flaw..
    std::optional<std::reference_wrapper<resolver>> c_res; // The current resolver..
  };

  class enum_flaw final : public flaw
  {
  public:
    enum_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, riddle::enum_expr var) noexcept;

    [[nodiscard]] const riddle::enum_expr &get_var() const noexcept { return var; }

  private:
    void compute_resolvers() override;

  private:
    riddle::enum_expr var;
  };

  class clause_flaw final : public flaw
  {
  public:
    clause_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<riddle::bool_expr> &&clause, const bool &exclusive = false) noexcept;

    [[nodiscard]] const std::vector<riddle::bool_expr> &get_clause() const noexcept { return clause; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<riddle::bool_expr> clause;
  };

  class disjunction_flaw final : public flaw
  {
  public:
    disjunction_flaw(basic_solver &slv, std::vector<std::reference_wrapper<resolver>> &&causes, std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) noexcept;

    [[nodiscard]] const std::vector<std::unique_ptr<riddle::conjunction>> &get_disjuncts() const noexcept { return disjuncts; }

  private:
    void compute_resolvers() override;

  private:
    std::vector<std::unique_ptr<riddle::conjunction>> disjuncts;
  };
} // namespace ratio
