#include <cassert>
#include "bool_flaw.hpp"
#include "solver.hpp"
#include "graph.hpp"

namespace ratio
{
    bool_flaw::bool_flaw(solver &s, std::vector<std::reference_wrapper<resolver>> &&causes, const utils::lit &l) noexcept : flaw(s, std::move(causes)), l(l) {}

    void bool_flaw::compute_resolvers()
    {
        switch (get_solver().get_sat().value(l))
        {
        case utils::True: // we add only the positive resolver (which is already active)
            get_solver().get_graph().new_resolver<choose_value>(*this, utils::rational::zero, l);
            break;
        case utils::False: // we add only the negative resolver (which is already active)
            get_solver().get_graph().new_resolver<choose_value>(*this, utils::rational::zero, !l);
            break;
        case utils::Undefined:
            get_solver().get_graph().new_resolver<choose_value>(*this, utils::rational::zero, l);
            get_solver().get_graph().new_resolver<choose_value>(*this, utils::rational::zero, !l);
            break;
        }
    }

    bool_flaw::choose_value::choose_value(bool_flaw &bf, const utils::rational &cost, const utils::lit &l) : resolver(bf, l, cost) {}

#ifdef ENABLE_VISUALIZATION
    json::json bool_flaw::get_data() const noexcept { return {{"type", "bool_flaw"}, {"lit", to_string(l)}}; }
#endif
} // namespace ratio
