#include "z3types.hpp"
#include "combinations.hpp"
#include <set>

namespace ratio
{
    z3component_type::z3component_type(z3solver &slv, std::string &&name) noexcept : riddle::component_type(slv, std::move(name)) {}

    void z3component_type::add(const std::vector<atom *> &atms, const z3::expr &e)
    {
        z3::expr_vector impl(static_cast<z3solver &>(get_core()).ctx);
        for (const auto &atm : atms)
            impl.push_back(atm->get_sigma() == 1);
        static_cast<z3solver &>(get_core()).slv.add(z3::implies(z3::mk_and(impl), e));
    }

    z3state_variable::z3state_variable(z3solver &slv) noexcept : z3component_type(slv, state_variable_kw) {}

    bool z3state_variable::solve_inconsistencies()
    { // we assign the atoms to the state-variables..
        std::unordered_map<const riddle::component *, std::map<utils::inf_rational, std::pair<std::vector<atom *>, std::vector<atom *>>>> sv_instances;
        for (const auto &[name, pred] : get_predicates())
            for (const auto &atm : pred->get_atoms())
                if (static_cast<atom &>(*atm).is_active())
                {
                    auto &tau = static_cast<riddle::enum_item &>(*atm->get(riddle::tau_kw)); // the atom's tau variable..
                    auto &sv = static_cast<riddle::component &>(get_core().enum_value(tau)); // the tau variable's value..
                    const auto start = get_core().arith_value(static_cast<riddle::arith_item &>(*atm->get(riddle::start_kw)));
                    const auto end = get_core().arith_value(static_cast<riddle::arith_item &>(*atm->get(riddle::end_kw)));
                    sv_instances[&sv][start].first.push_back(&static_cast<atom &>(*atm));
                    sv_instances[&sv][end].second.push_back(&static_cast<atom &>(*atm));
                }

        bool has_conflicts = false;
        // we detect inconsistencies for each of the state-variable instances..
        for ([[maybe_unused]] const auto &[_, atms] : sv_instances)
        {
            // we scroll through the timeline looking for inconsistencies..
            std::set<atom *> overlapping_atoms;
            for (const auto &[pulse, start_end] : atms)
            {
                overlapping_atoms.insert(start_end.first.begin(), start_end.first.end());
                for (const auto &atm : start_end.second)
                    overlapping_atoms.erase(atm);

                // if there are more than one atom in the set of overlapping atoms, we have an inconsistency..
                if (overlapping_atoms.size() > 1)
                {
                    has_conflicts = true;
                    // we consider all the pairs of atoms in the Minimal Conflict Sets (MCSs)..
                    for (const auto &as : utils::combinations(std::vector<atom *>(overlapping_atoms.cbegin(), overlapping_atoms.cend()), 2))
                    {
                        auto ress = get_core().new_or({get_core().new_le(std::dynamic_pointer_cast<riddle::arith_item>(as[0]->get(riddle::end_kw)), std::dynamic_pointer_cast<riddle::arith_item>(as[1]->get(riddle::start_kw))),
                                                       get_core().new_le(std::dynamic_pointer_cast<riddle::arith_item>(as[1]->get(riddle::end_kw)), std::dynamic_pointer_cast<riddle::arith_item>(as[0]->get(riddle::start_kw))),
                                                       get_core().new_not(get_core().new_eq(as[0]->get(riddle::tau_kw), as[1]->get(riddle::tau_kw)))});
                        add(as, static_cast<bool_item &>(*ress).get_expr());
                    }
                }
            }
        }

        return has_conflicts;
    }

    z3reusable_resource::z3reusable_resource(z3solver &slv) noexcept : z3component_type(slv, reusable_resource_kw) {}

    bool z3reusable_resource::solve_inconsistencies()
    { // we assign the atoms to the reusable-resources..
        std::unordered_map<riddle::component *, std::map<utils::inf_rational, std::pair<std::vector<atom *>, std::vector<atom *>>>> sv_instances;
        for (const auto &[name, pred] : get_predicates())
            for (const auto &atm : pred->get_atoms())
                if (static_cast<atom &>(*atm).is_active())
                {
                    auto &tau = static_cast<riddle::enum_item &>(*atm->get(riddle::tau_kw)); // the atom's tau variable..
                    auto &sv = static_cast<riddle::component &>(get_core().enum_value(tau)); // the tau variable's value..
                    const auto start = get_core().arith_value(static_cast<riddle::arith_item &>(*atm->get(riddle::start_kw)));
                    const auto end = get_core().arith_value(static_cast<riddle::arith_item &>(*atm->get(riddle::end_kw)));
                    sv_instances[&sv][start].first.push_back(&static_cast<atom &>(*atm));
                    sv_instances[&sv][end].second.push_back(&static_cast<atom &>(*atm));
                }

        bool has_conflicts = false;
        // we detect inconsistencies for each of the reusable-resource instances..
        for ([[maybe_unused]] auto &[rr, atms] : sv_instances)
        {
            // we scroll through the timeline looking for inconsistencies..
            auto capacity = get_core().arith_value(static_cast<riddle::arith_item &>(*rr->get(reusable_resource_capacity_kw)));
            std::set<atom *> overlapping_atoms;
            for (const auto &[pulse, start_end] : atms)
            {
                overlapping_atoms.insert(start_end.first.begin(), start_end.first.end());
                for (const auto &atm : start_end.second)
                    overlapping_atoms.erase(atm);

                utils::inf_rational c_usage; // the concurrent resource usage..
                for (const auto &a : overlapping_atoms)
                    c_usage += get_core().arith_value(static_cast<riddle::arith_item &>(*a->get(reusable_resource_amount_kw)));

                // if the resource usage exceeds the resource capacity, we have a conflict..
                if (c_usage > capacity)
                {
                    has_conflicts = true;
                    // we extract the Minimal Conflict Sets (MCSs)..
                    // we sort the overlapping atoms, according to their resource usage, in descending order..
                    std::vector<atom *> inc_atoms(overlapping_atoms.cbegin(), overlapping_atoms.cend());
                    std::sort(inc_atoms.begin(), inc_atoms.end(), [this](const auto &atm0, const auto &atm1)
                              { return get_core().arith_value(static_cast<riddle::arith_item &>(*atm0->get(reusable_resource_amount_kw))) > get_core().arith_value(static_cast<riddle::arith_item &>(*atm1->get(reusable_resource_amount_kw))); });

                    utils::inf_rational mcs_usage;       // the concurrent mcs resource usage..
                    auto mcs_begin = inc_atoms.cbegin(); // the beginning of the current mcs..
                    auto mcs_end = inc_atoms.cbegin();   // the end of the current mcs..
                    while (mcs_end != inc_atoms.cend())
                    {
                        // we increase the size of the current mcs..
                        while (mcs_usage <= capacity && mcs_end != inc_atoms.cend())
                        {
                            mcs_usage += get_core().arith_value(static_cast<riddle::arith_item &>(*(*mcs_end)->get(reusable_resource_amount_kw)));
                            ++mcs_end;
                        }

                        if (mcs_usage > capacity)
                        {
                            std::set<atom *> mcs(mcs_begin, mcs_end); // the MCS..
                            std::vector<riddle::bool_expr> exprs;
                            for (const auto &as : utils::combinations(std::vector<atom *>(mcs.cbegin(), mcs.cend()), 2))
                            {
                                exprs.push_back(get_core().new_le(std::dynamic_pointer_cast<riddle::arith_item>(as[0]->get(riddle::end_kw)), std::dynamic_pointer_cast<riddle::arith_item>(as[1]->get(riddle::start_kw))));
                                exprs.push_back(get_core().new_le(std::dynamic_pointer_cast<riddle::arith_item>(as[1]->get(riddle::end_kw)), std::dynamic_pointer_cast<riddle::arith_item>(as[0]->get(riddle::start_kw))));
                                exprs.push_back(get_core().new_not(get_core().new_eq(as[0]->get(riddle::tau_kw), as[1]->get(riddle::tau_kw))));
                            }
                            std::vector<riddle::arith_expr> usage_exprs;
                            for (const auto &a : mcs)
                                usage_exprs.push_back(std::dynamic_pointer_cast<riddle::arith_item>(a->get(reusable_resource_amount_kw)));
                            exprs.push_back(get_core().new_le(get_core().new_sum(std::move(usage_exprs)), std::dynamic_pointer_cast<riddle::arith_item>(rr->get(reusable_resource_capacity_kw))));
                            auto ress = get_core().new_or(std::move(exprs));
                            add(std::vector<atom *>(mcs.cbegin(), mcs.cend()), static_cast<bool_item &>(*ress).get_expr());
                        }

                        // we decrease the size of the current mcs..
                        mcs_usage -= get_core().arith_value(static_cast<riddle::arith_item &>(*(*mcs_begin)->get(reusable_resource_amount_kw)));
                        assert(mcs_usage <= capacity);
                        ++mcs_begin;
                    }
                }
            }
        }

        return has_conflicts;
    }
} // namespace ratio