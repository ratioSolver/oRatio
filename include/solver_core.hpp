#pragma once

#include "core.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"

#ifdef ORATIO_ENABLE_LISTENERS
#define NEW_FLAW(f) flaw_created(f)
#define NEW_RESOLVER(r) resolver_created(r)
#else
#define NEW_FLAW(f)
#define NEW_RESOLVER(r)
#endif

namespace ratio
{
  class solver_core;
  class resolver;

  class flaw
  {
  public:
    flaw(solver_core &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept;
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

  private:
    virtual void compute_resolvers() = 0;

  protected:
    solver_core &slv;                                     // The solver managing this flaw..
    std::vector<std::reference_wrapper<resolver>> causes; // The causes of this flaw..
  };

  class resolver
  {
    friend class solver_core;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

  private:
    virtual void apply() = 0;

  protected:
    flaw &flw;                                                                 // The flaw this resolver addresses..
    utils::rational intrinsic_cost;                                            // The intrinsic cost of applying this resolver..
    linspire::constraint cnst;                                                 // The constraint associated with this resolver..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints associated with this resolver..
  };

  class solver_core : public riddle::core
  {
  public:
    solver_core(std::string_view name = "oRatio") noexcept;

    [[nodiscard]] riddle::bool_expr new_bool() override;
    [[nodiscard]] riddle::bool_expr new_bool(const bool value) override;
    [[nodiscard]] utils::lbool bool_value(const riddle::bool_term &expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_int() override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE value) override;
    [[nodiscard]] riddle::arith_expr new_int(const INT_TYPE lb, const INT_TYPE ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_int(const INT_TYPE lb, const INT_TYPE ub) override;

    [[nodiscard]] riddle::arith_expr new_real() override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&value) override;
    [[nodiscard]] riddle::arith_expr new_real(utils::rational &&lb, utils::rational &&ub) override;
    [[nodiscard]] riddle::arith_expr new_uncertain_real(utils::rational &&lb, utils::rational &&ub) override;

    [[nodiscard]] riddle::arith_expr new_time() override;
    [[nodiscard]] riddle::arith_expr new_time(utils::rational &&value) override;

    [[nodiscard]] utils::inf_rational arith_value(const riddle::arith_term &expr) const noexcept override;

    [[nodiscard]] riddle::string_expr new_string() override;
    [[nodiscard]] riddle::string_expr new_string(std::string &&value) override;
    [[nodiscard]] std::string string_value(const riddle::string_term &expr) const noexcept override;

    [[nodiscard]] std::vector<riddle::expr> enum_value(const riddle::enum_term &expr) const noexcept override;

    [[nodiscard]] riddle::arith_expr new_negation(riddle::arith_expr xpr) override;

    [[nodiscard]] riddle::arith_expr new_sum(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_subtraction(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_product(std::vector<riddle::arith_expr> &&xprs) override;
    [[nodiscard]] riddle::arith_expr new_division(std::vector<riddle::arith_expr> &&xprs) override;

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

    virtual void solve() = 0;

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

  protected:
    [[nodiscard]] bool execute(const riddle::bool_expr &expr) noexcept;

    void add_constraint(arc_consistency::constraint &c) noexcept;

    std::vector<std::reference_wrapper<resolver>> get_causes() const noexcept;

  private:
    [[nodiscard]] riddle::atom_state get_atom_state(const riddle::atom_term &atom) const noexcept override;

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

  protected:
    arc_consistency::solver ac_slv; // The arc consistency solver..
    linspire::solver lin_slv;       // The linear programming solver..
  private:
    std::vector<std::unique_ptr<flaw>> flaws;              // The set of flaws
    std::vector<std::unique_ptr<resolver>> resolvers;      // The set of resolvers
    std::optional<std::reference_wrapper<flaw>> c_flaw;    // The current flaw..
    std::optional<std::reference_wrapper<resolver>> c_res; // The current resolver..
  };
} // namespace ratio
