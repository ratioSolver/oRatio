#include "bool_flaw.h"

namespace ratio
{
    bool_flaw::bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, riddle::expr &b_item) : flaw(s, std::move(causes)), b_item(b_item) {}

    void bool_flaw::compute_resolvers()
    {
        add_resolver(new choose_value(*this, true));
        add_resolver(new choose_value(*this, false));
    }

    json::json bool_flaw::get_data() const noexcept
    {
        json::json data = flaw::get_data();
        data["type"] = "bool_flaw";
        data["item"] = get_id(*b_item);
        return data;
    }

    bool_flaw::choose_value::choose_value(bool_flaw &ef, bool value) : resolver(ef, utils::rational(1, 2)), value(value) {}

    void bool_flaw::choose_value::apply() {}

    json::json bool_flaw::choose_value::get_data() const noexcept
    {
        json::json data = resolver::get_data();
        data["type"] = "choose_value";
        data["value"] = value;
        return data;
    }
} // namespace ratio
