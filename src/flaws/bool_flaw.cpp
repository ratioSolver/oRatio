#include "bool_flaw.h"
#include "solver.h"
#include "item.h"

namespace ratio::solver
{
    bool_flaw::bool_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::bool_item &b_itm) : flaw(slv, std::move(causes), true), b_itm(b_itm) {}

    ORATIOSOLVER_EXPORT json::json bool_flaw::get_data() const noexcept
    {
        json::json j_f;
        j_f["type"] = "bool";
        return j_f;
    }

    void bool_flaw::compute_resolvers()
    {
        add_resolver(std::make_unique<choose_value>(semitone::rational(1, 2), *this, b_itm.get_value()));
        add_resolver(std::make_unique<choose_value>(semitone::rational(1, 2), *this, !b_itm.get_value()));
    }

    bool_flaw::choose_value::choose_value(semitone::rational cst, bool_flaw &bl_flaw, const semitone::lit &val) : resolver(val, cst, bl_flaw) {}

    ORATIOSOLVER_EXPORT json::json bool_flaw::choose_value::get_data() const noexcept { return json::object(); }

    void bool_flaw::choose_value::apply() {}
} // namespace ratio::solver