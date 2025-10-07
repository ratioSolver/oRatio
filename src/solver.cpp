#include "solver.hpp"

namespace ratio
{
    solver::solver(std::string_view name) noexcept : riddle::core(name), assigns() {}

    riddle::bool_expr solver::new_bool() { return new_bool(utils::Undefined); }

    riddle::bool_expr solver::new_bool(const bool value)
    {
        auto var = riddle::core::new_bool();
        assigns.push_back(value ? utils::True : utils::False);
        return var;
    }

    utils::lbool solver::bool_value(const riddle::bool_term &expr) const noexcept { return assigns.at(expr.get_id()); }
} // namespace ratio
