#include "solver.h"

namespace ratio
{
    solver::solver() : theory(new semitone::sat_core()), lra_th(sat), ov_th(sat), idl_th(sat), rdl_th(sat) {}
} // namespace ratio
