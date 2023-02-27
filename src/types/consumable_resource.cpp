#include "consumable_resource.h"
#include "solver.h"

namespace ratio
{
    consumable_resource::consumable_resource(riddle::scope &scp) : smart_type(scp, CONSUMABLE_RESOURCE_NAME) {}
} // namespace ratio
