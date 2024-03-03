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

    std::shared_ptr<riddle::item> solver::new_bool() noexcept { return std::make_shared<bool_item>(get_bool_type(), utils::lit(sat->new_var())); }
    std::shared_ptr<riddle::item> solver::new_bool(bool value) noexcept { return std::make_shared<bool_item>(get_bool_type(), value ? utils::TRUE_lit : utils::FALSE_lit); }
    std::shared_ptr<riddle::item> solver::new_int() noexcept { return std::make_shared<arith_item>(get_int_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_int(INTEGER_TYPE value) noexcept { return std::make_shared<arith_item>(get_int_type(), utils::lin(utils::rational(value))); }
    std::shared_ptr<riddle::item> solver::new_real() noexcept { return std::make_shared<arith_item>(get_real_type(), utils::lin(lra.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_real(const utils::rational &value) noexcept { return std::make_shared<arith_item>(get_real_type(), utils::lin(value)); }
    std::shared_ptr<riddle::item> solver::new_time() noexcept { return std::make_shared<arith_item>(get_time_type(), utils::lin(rdl.new_var(), utils::rational::one)); }
    std::shared_ptr<riddle::item> solver::new_time(const utils::rational &value) noexcept { return std::make_shared<arith_item>(get_time_type(), utils::lin(value)); }
    std::shared_ptr<riddle::item> solver::new_string() noexcept { return std::make_shared<string_item>(get_string_type()); }
    std::shared_ptr<riddle::item> solver::new_string(const std::string &value) noexcept { return std::make_shared<string_item>(get_string_type(), value); }
    std::shared_ptr<riddle::item> solver::new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) { return std::make_shared<enum_item>(tp, ov.new_var(std::move(values))); }
} // namespace ratio