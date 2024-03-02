#include "solver.hpp"
#include "init.hpp"
#include "sat_core.hpp"
#include "logging.hpp"

namespace ratio
{
    solver::solver(const std::string &name, bool i) noexcept : theory(std::make_shared<semitone::sat_core>()), name(name), lra(get_sat_ptr()), idl(get_sat_ptr()), rdl(get_sat_ptr()), ov(get_sat_ptr()), gr(*this)
    {
        if (i)
            init();
    }

    void solver::init() noexcept
    {
        LOG_INFO("Initializing solver " << name);
    }

    std::shared_ptr<riddle::item> solver::new_bool() noexcept { return std::make_shared<bool_item>(get_bool_type(), sat->new_var()); }
    std::shared_ptr<riddle::item> solver::new_bool(bool value) noexcept { return std::make_shared<bool_item>(get_bool_type(), value ? semitone::TRUE_lit : semitone::FALSE_lit); }
} // namespace ratio