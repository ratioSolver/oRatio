#pragma once

#include "core.hpp"
#include "linspire.hpp"
#include "arc_consistency.hpp"

namespace ratio
{
  class solver_core;
  class resolver;

  class flaw
  {
    friend class solver_core;
    friend class resolver;

  public:
    flaw(solver_core &slv, std::vector<std::reference_wrapper<resolver>> &&causes) noexcept;
    flaw(const flaw &) = delete;
    virtual ~flaw() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] bool is_expanded() const noexcept { return expanded; }

    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_causes() const noexcept { return causes; }
    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_resolvers() const noexcept { return resolvers; }
    [[nodiscard]] const std::vector<std::reference_wrapper<resolver>> &get_supports() const noexcept { return supports; }

    [[nodiscard]] const utils::rational &get_estimated_cost() const noexcept { return est_cost; }

    [[nodiscard]] virtual json::json to_json() const;

  protected:
    [[nodiscard]] solver_core &get_solver() const noexcept { return slv; }
    [[nodiscard]] linspire::solver &get_lin() const noexcept;
    [[nodiscard]] arc_consistency::solver &get_ac() const noexcept;

  private:
    virtual void compute_resolvers() = 0;

  private:
    solver_core &slv;                                              // The solver managing this flaw..
    bool expanded = false;                                         // Whether the flaw has been expanded..
    std::vector<std::reference_wrapper<resolver>> causes;          // The causes of this flaw..
    std::vector<std::reference_wrapper<resolver>> resolvers;       // The resolvers for this flaw..
    std::vector<std::reference_wrapper<resolver>> supports;        // The resolvers supported by this flaw..
    utils::rational est_cost = utils::rational::positive_infinite; // The estimated cost to resolve this flaw..
  };

  class resolver
  {
    friend class solver_core;
    friend class flaw;

  public:
    resolver(flaw &flw, utils::rational &&intrinsic_cost) noexcept;
    resolver(const resolver &) = delete;
    virtual ~resolver() = default;

    [[nodiscard]] uintptr_t get_id() const noexcept { return reinterpret_cast<uintptr_t>(this); }

    [[nodiscard]] flaw &get_flaw() const noexcept { return flw; }

    [[nodiscard]] const utils::rational &get_intrinsic_cost() const noexcept { return intrinsic_cost; }

    [[nodiscard]] utils::rational get_estimated_cost() const noexcept;

    [[nodiscard]] const std::vector<std::reference_wrapper<flaw>> &get_preconditions() const noexcept { return preconditions; }

    [[nodiscard]] virtual json::json to_json() const;

  protected:
    [[nodiscard]] solver_core &get_solver() const noexcept { return flw.slv; }
    [[nodiscard]] linspire::solver &get_lin() const noexcept { return flw.get_lin(); }
    [[nodiscard]] linspire::constraint &get_lin_constraint() noexcept { return cnst; }
    [[nodiscard]] arc_consistency::solver &get_ac() const noexcept { return flw.get_ac(); }
    void add_ac_constraint(arc_consistency::constraint &c) noexcept { ac_cnsts.push_back(c); }
    void execute(const riddle::bool_expr &expr);

  private:
    virtual void apply() = 0;

  protected:
    flaw &flw;                            // The flaw this resolver addresses..
    const utils::rational intrinsic_cost; // The intrinsic cost of applying this resolver..

  private:
    linspire::constraint cnst;                                                 // The constraint associated with this resolver..
    std::vector<std::reference_wrapper<arc_consistency::constraint>> ac_cnsts; // The arc consistency constraints associated with this resolver..
    std::vector<std::reference_wrapper<flaw>> preconditions;                   // The preconditions of this resolver..
  };

  class solver_core : public riddle::core
  {
    friend class flaw;
    friend class resolver;

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
#ifdef ORATIO_ENABLE_LISTENERS
      flaw_created(f_ref);
#endif
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
#ifdef ORATIO_ENABLE_LISTENERS
      resolver_created(r_ref);
#endif
      resolvers.emplace_back(std::move(r));
      return r_ref;
    }

    virtual void add_causal_link(flaw &f, resolver &r) noexcept;

    virtual void solve() = 0;

    [[nodiscard]] bool match(riddle::term &lhs, riddle::term &rhs) const;

  protected:
    [[nodiscard]] bool execute(const riddle::bool_expr &expr) noexcept;

    void add_constraint(arc_consistency::constraint &c) noexcept;

    std::vector<std::reference_wrapper<resolver>> get_causes() const noexcept;

    void compute_resolvers(flaw &flw) noexcept;

    void apply_resolver(resolver &res);

    void retract_resolver(resolver &res) noexcept;

    void set_flaw_cost(flaw &f, const utils::rational &cost) noexcept;

  private:
    [[nodiscard]] riddle::atom_state get_atom_state(const riddle::atom_term &atom) const noexcept override;

#ifdef ORATIO_ENABLE_LISTENERS
  private:
    /**
     * @brief This function is called when the state of the solver changes.
     *
     * This function should be overridden by derived classes to handle the state change event.
     *
     * @note This is a virtual function and can be overridden by derived classes.
     */
    virtual void state_changed() noexcept {}

    /**
     * @brief Notifies that a new flaw has been created.
     *
     * This function is called whenever a new flaw is created in the solver.
     *
     * @param f The newly created flaw.
     */
    virtual void flaw_created([[maybe_unused]] const flaw &f) noexcept {}

    /**
     * @brief Notifies when the cost of a flaw has changed.
     *
     * This function is called when the cost of a flaw has changed. It is a virtual function that can be overridden by derived classes to perform specific actions when a flaw's cost changes.
     *
     * @param f The flaw whose cost has changed.
     */
    virtual void flaw_cost_changed([[maybe_unused]] const flaw &f) {}

    /**
     * @brief Notifies that a new resolver has been created.
     *
     * This function is called whenever a new resolver is created in the solver.
     *
     * @param r The newly created resolver.
     */
    virtual void resolver_created([[maybe_unused]] const resolver &r) noexcept {}

    /**
     * @brief Notifies about the current flaw being processed.
     *
     * This function is called to inform about the current flaw being processed in the solver.
     *
     * @param f The current flaw being processed.
     */
    virtual void current_flaw([[maybe_unused]] std::optional<std::reference_wrapper<ratio::flaw>> f) noexcept {}

    /**
     * @brief Notifies about the current resolver being applied.
     *
     * This function is called to inform about the current resolver being applied in the solver.
     *
     * @param r The current resolver being applied.
     */
    virtual void current_resolver([[maybe_unused]] std::optional<std::reference_wrapper<ratio::resolver>> r) noexcept {}

    /**
     * @brief Notifies when a causal link has been added.
     *
     * This function is called when a causal link has been added. It is a virtual function that can be overridden by derived classes to perform specific actions when a causal link is added.
     *
     * @param f The flaw that is the source of the causal link.
     * @param r The resolver that is the destination of the causal link.
     */
    virtual void causal_link_added([[maybe_unused]] const flaw &f, [[maybe_unused]] const resolver &r) {}
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