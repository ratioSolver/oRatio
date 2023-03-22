#include "enum_flaw.h"
#include "solver.h"
#include "ov_theory.h"

namespace ratio
{
    enum_flaw::enum_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> causes, enum_item &ei) : flaw(s, std::move(causes), true), ei(ei) {}

    void enum_flaw::compute_resolvers()
    { // we add a resolver for each possible value of the enum..
        auto dom = get_solver().domain(&ei);
        for (auto &v : dom)
        {
            auto &val = dynamic_cast<utils::enum_val &>(*v);
            add_resolver(new enum_resolver(*this, get_solver().get_ov_theory().allows(ei.get_var(), val), utils::rational(1, static_cast<utils::I>(dom.size())), val));
        }
    }

    json::json enum_flaw::get_data() const noexcept { return {{"type", "enum"}}; }

    enum_flaw::enum_resolver::enum_resolver(enum_flaw &ef, const semitone::lit &rho, const utils::rational &cost, utils::enum_val &val) : resolver(ef, rho, cost), val(val) {}

    json::json enum_flaw::enum_resolver::get_data() const noexcept
    {
        json::json j{{"type", "assignment"}, {"value", value(dynamic_cast<riddle::item &>(val))}};
#ifdef COMPUTE_NAMES
        auto name = get_solver().guess_name(dynamic_cast<riddle::item &>(val));
        if (!name.empty())
            j["name"] = name;
#endif
        return j;
    }
} // namespace ratio
