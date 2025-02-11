#include "stflaws.hpp"

namespace ratio
{
    stflaw::stflaw(stsolver &slv, std::vector<utils::ref_wrapper<resolver>> &&causes) noexcept : flaw(slv, std::move(causes)), phi(compute_phi(slv, get_causes())), pos(slv.ctx.real_const(("p" + std::to_string(slv.position_count++)).c_str()))
    {
        for (const auto &cause : causes) // we impose the position constraint (i.e., the flaw must be before its causes) to avoid causality loops..
            slv.slv.add(st::implies(phi, pos <= static_cast<stflaw &>(cause->get_flaw()).pos - 1));
    }

    utils::lit stflaw::compute_phi(stsolver &slv, const std::vector<utils::ref_wrapper<resolver>> &causes) noexcept
    {
        st::expr_vector cs(slv.ctx);
        for (const auto &cause : causes)
            cs.push_back(static_cast<stresolver &>(*cause).get_rho());
        return st::mk_and(cs);
    }

    void stflaw::expanded_flaw()
    {
        st::expr_vector rs(static_cast<stsolver &>(get_graph()).ctx);
        for (const auto &resolver : get_resolvers())
            rs.push_back(static_cast<stresolver &>(*resolver).get_rho());
        static_cast<stsolver &>(get_graph()).slv.add(st::implies(phi, st::mk_or(rs))); // if the flaw is active, then at least one resolver must be active..
    }
} // namespace ratio
