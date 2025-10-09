#pragma once

#include "core.hpp"
#include "lit.hpp"
#include "linspire.hpp"

#ifdef ENABLE_API
#define STATE_CHANGED() state_changed()
#define NEW_FLAW(f) flaw_created(f)
#define FLAW_STATE_CHANGED(f) flaw_state_changed(f)
#define FLAW_COST_CHANGED(f) flaw_cost_changed(f)
#define FLAW_POSITION_CHANGED(f) flaw_position_changed(f)
#define NEW_RESOLVER(r) resolver_created(r)
#define RESOLVER_STATE_CHANGED(r) resolver_state_changed(r)
#define NEW_CAUSAL_LINK(f, r) causal_link_added(f, r)
#define CURRENT_FLAW(f) current_flaw(f)
#define CURRENT_RESOLVER(r) current_resolver(r)
#else
#define STATE_CHANGED()
#define NEW_FLAW(f)
#define FLAW_STATE_CHANGED(f)
#define FLAW_COST_CHANGED(f)
#define FLAW_POSITION_CHANGED(f)
#define NEW_RESOLVER(r)
#define RESOLVER_STATE_CHANGED(r)
#define NEW_CAUSAL_LINK(f, r)
#define CURRENT_FLAW(f)
#define CURRENT_RESOLVER(r)
#endif

namespace ratio
{
  class flaw;
  class resolver;

  class solver : public riddle::core
  {
  public:
    solver(std::string_view name = "oRatio") noexcept;

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

    [[nodiscard]] riddle::enum_expr new_enum(riddle::component_type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;
    [[nodiscard]] std::vector<std::reference_wrapper<utils::enum_val>> enum_value(const riddle::enum_term &expr) const noexcept override;

  private:
    /**
     * @brief Create a new propositional variable
     *
     * @return The new variable.
     */
    [[nodiscard]] utils::var mk_var() noexcept;

    /**
     * @brief Return the value of a variable.
     *
     * @param x The variable.
     * @return The value of the variable.
     */
    [[nodiscard]] utils::lbool value(const utils::var &x) const noexcept { return assigns.at(x); }
    /**
     * @brief Return the value of a literal.
     *
     * @param p The literal.
     * @return The value of the literal.
     */
    [[nodiscard]] utils::lbool value(const utils::lit &p) const noexcept
    {
      switch (value(variable(p)))
      {
      case utils::True:
        return sign(p) ? utils::True : utils::False;
      case utils::False:
        return sign(p) ? utils::False : utils::True;
      default:
        return utils::Undefined;
      }
    }

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
      return f_ref;
    }

  private:
    std::vector<utils::lbool> assigns;                     // for each variable, the current assignment..
    linspire::solver lin_slv;                              // the linear solver..
    std::optional<std::reference_wrapper<resolver>> c_res; // the current resolver..
  };
} // namespace ratio
