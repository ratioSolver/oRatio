#include "state_variable.h"
#include "solver.h"

namespace ratio
{
    state_variable::state_variable(riddle::scope &scp) : smart_type(scp, STATE_VARIABLE_NAME) {}
} // namespace ratio