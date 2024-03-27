#include "smart_type.hpp"
#include "solver.hpp"

namespace ratio
{
    smart_type::smart_type(scope &parent, const std::string &name) : riddle::component_type(parent, name), slv(dynamic_cast<solver &>(parent.get_core())) {}
} // namespace ratio
