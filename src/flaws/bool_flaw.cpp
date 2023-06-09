#include "bool_flaw.h"
#include "solver.h"

namespace ratio
{
    bool_flaw::bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, riddle::expr &b_item) : flaw(s, std::move(causes)), b_item(b_item) {}

    void bool_flaw::compute_resolvers()
    {
        auto &bi = static_cast<bool_item &>(*b_item);
        switch (get_solver().get_sat_core().value(bi.get_lit()))
        {
        case utils::True: // we add only the positive resolver (which is already active)
            add_resolver(new choose_value(*this, bi.get_lit()));
            break;
        case utils::False: // we add only the negative resolver (which is already active)
            add_resolver(new choose_value(*this, !bi.get_lit()));
            break;
        case utils::Undefined: // we add both resolvers
            add_resolver(new choose_value(*this, bi.get_lit()));
            add_resolver(new choose_value(*this, !bi.get_lit()));
            break;
        }
    }

    json::json bool_flaw::get_data() const noexcept { return {{"type", "bool"}, {"item", get_id(*b_item)}}; }

    bool_flaw::choose_value::choose_value(bool_flaw &ef, const semitone::lit &rho) : resolver(ef, rho, utils::rational(1, 2)) {}

    void bool_flaw::choose_value::apply() {}

    json::json bool_flaw::choose_value::get_data() const noexcept { return {{"type", "choose_value"}}; }
} // namespace ratio
