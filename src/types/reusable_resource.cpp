#include "reusable_resource.h"

namespace ratio
{
    reusable_resource::reusable_resource(riddle::scope &scp) : smart_type(scp, REUSABLE_RESOURCE_NAME) {}
} // namespace ratio