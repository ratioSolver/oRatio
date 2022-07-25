#include "solver.h"
#include "atom.h"

namespace ratio::solver
{
    solver::solver() : sat_cr(), lra_th(sat_cr), ov_th(sat_cr), idl_th(sat_cr), rdl_th(sat_cr) {}

    ORATIO_EXPORT ratio::core::expr solver::new_bool() noexcept { return std::make_shared<ratio::core::bool_item>(get_bool_type(), semitone::lit(sat_cr.new_var())); }
    ORATIO_EXPORT ratio::core::expr solver::new_int() noexcept { return std::make_shared<ratio::core::arith_item>(get_int_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_real() noexcept { return std::make_shared<ratio::core::arith_item>(get_real_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_time_point() noexcept { return std::make_shared<ratio::core::arith_item>(get_time_type(), semitone::lin(lra_th.new_var(), semitone::rational::ONE)); }
    ORATIO_EXPORT ratio::core::expr solver::new_string() noexcept { return std::make_shared<ratio::core::string_item>(get_string_type(), ""); }
} // namespace ratio::solver
