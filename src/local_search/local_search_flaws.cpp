#include "local_search_flaws.hpp"
#include "items.hpp"
#include "combinations.hpp"
#include <cassert>

namespace ratio
{
    ls_flaw::ls_flaw(solver &cr, std::vector<std::shared_ptr<riddle::resolver>> &&causes, const utils::lit &phi) : flaw(cr, std::move(causes)), phi(phi) {}

    enum_flaw::enum_flaw(solver &slv, std::vector<std::shared_ptr<riddle::resolver>> &&causes, const utils::lit &phi, riddle::component_type &tp, std::vector<riddle::expr> &&values, utils::var ev) noexcept : ls_flaw(slv, std::move(causes), phi), var(std::make_shared<riddle::enum_item>(*this, tp, std::move(values), ev)) {}

    utils::rational enum_flaw::get_estimated_cost() const noexcept { return get_core().enum_value(*var).size(); }

    void enum_flaw::compute_resolvers()
    { // Create a resolver for each possible value..
        for (auto val : var->get_values())
            new_resolver<select_value>(*this, utils::lit(), val);
    }

    select_value::select_value(enum_flaw &flw, const utils::lit &rho, riddle::expr val) noexcept : riddle::resolver(flw, utils::rational(1)), ls_resolver(flw, utils::rational(1), rho), riddle::select_value(flw, std::move(val)) {}
    bool select_value::apply() noexcept { return ratio::resolver::get_flaw().get_core().assert_expr(ratio::resolver::get_flaw().get_core().new_eq(static_cast<enum_flaw &>(ratio::resolver::flw).get_var(), get_value())); }
} // namespace ratio
