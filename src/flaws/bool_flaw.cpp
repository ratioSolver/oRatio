#include "bool_flaw.h"
#include "solver.h"
#include "item.h"

namespace ratio::solver
{
    bool_flaw::bool_flaw(solver &slv, std::vector<resolver *> causes, ratio::core::bool_item &b_itm) : flaw(slv, std::move(causes), true), b_itm(b_itm) {}

    void bool_flaw::compute_resolvers()
    {
        add_resolver(std::make_unique<choose_value>(semitone::rational(1, 2), *this, b_itm.get_value()));
        add_resolver(std::make_unique<choose_value>(semitone::rational(1, 2), *this, !b_itm.get_value()));
    }

    bool_flaw::choose_value::choose_value(semitone::rational cst, bool_flaw &bl_flaw, const semitone::lit &val) : resolver(val, cst, bl_flaw) {}

    void bool_flaw::choose_value::apply() {}
} // namespace ratio::solver