#include <cassert>
#include "enum_flaw.hpp"
#include "graph.hpp"
#ifdef ENABLE_API
#include "solver_api.hpp"
#endif

namespace ratio
{
    enum_flaw::enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, std::shared_ptr<riddle::enum_item> item) noexcept : flaw(s, std::move(causes), true, false), itm(item) { assert(itm != nullptr); }

    void enum_flaw::compute_resolvers()
    { // we add a resolver for each possible value of the enum..
        auto dom = get_solver().domain(*itm);
        for (auto &v : dom)
        {
            auto &val = dynamic_cast<utils::enum_val &>(v.get());
            get_solver().get_graph().new_resolver<choose_value>(*this, get_solver().get_ov_theory().allows(itm->get_value(), val), utils::rational::one / dom.size(), val);
        }
    }

    enum_flaw::choose_value::choose_value(enum_flaw &bf, const utils::lit &rho, const utils::rational &cost, const utils::enum_val &val) : resolver(bf, rho, cost), val(val) {}

#ifdef ENABLE_API
    json::json enum_flaw::get_data() const noexcept { return {{"type", "enum"}}; }

    json::json enum_flaw::choose_value::get_data() const noexcept
    {
        json::json j{{"type", "assignment"}, {"value", value(dynamic_cast<const riddle::item &>(val))}};
        auto name = get_flaw().get_solver().guess_name(dynamic_cast<const riddle::item &>(val));
        if (!name.empty())
            j["name"] = name;
        return j;
    }
#endif
} // namespace ratio
