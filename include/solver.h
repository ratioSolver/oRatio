#pragma once

#include "oratio_export.h"
#include "core.h"
#include "sat_stack.h"
#include "lra_theory.h"
#include "ov_theory.h"
#include "idl_theory.h"
#include "rdl_theory.h"

namespace ratio::solver
{
  class solver : public ratio::core::core
  {
  public:
    solver();
    ~solver() = default;

    ORATIO_EXPORT ratio::core::expr new_bool() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_int() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_real() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_time_point() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_string() noexcept override;
    ORATIO_EXPORT ratio::core::expr new_enum(ratio::core::type &tp, const std::vector<ratio::core::expr> &allowed_vals) override;

    /**
     * @brief Get the sat core.
     *
     * @return semitone::sat_core& The sat core.
     */
    inline semitone::sat_core &get_sat_core() noexcept { return sat_cr; }
    /**
     * @brief Get the linear-real-arithmetic theory.
     *
     * @return semitone::lra_theory& The linear-real-arithmetic theory.
     */
    inline semitone::lra_theory &get_lra_theory() noexcept { return lra_th; }
    /**
     * @brief Get the object-variable theory.
     *
     * @return semitone::ov_theory& The object-variable theory.
     */
    inline semitone::ov_theory &get_ov_theory() noexcept { return ov_th; }
    /**
     * @brief Get the integer difference logic theory.
     *
     * @return semitone::idl_theory& The integer difference logic theory.
     */
    inline semitone::idl_theory &get_idl_theory() noexcept { return idl_th; }
    /**
     * @brief Get the real difference logic theory.
     *
     * @return semitone::rdl_theory& The real difference logic theory.
     */
    inline semitone::rdl_theory &get_rdl_theory() noexcept { return rdl_th; }

  private:
    semitone::sat_core sat_cr;   // the sat core..
    semitone::lra_theory lra_th; // the linear-real-arithmetic theory..
    semitone::ov_theory ov_th;   // the object-variable theory..
    semitone::idl_theory idl_th; // the integer difference logic theory..
    semitone::rdl_theory rdl_th; // the real difference logic theory..
  };
} // namespace ratio::solver
