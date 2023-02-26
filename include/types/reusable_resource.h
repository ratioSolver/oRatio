#pragma once

#include "smart_type.h"

#define REUSABLE_RESOURCE_NAME "ReusableResource"
#define REUSABLE_RESOURCE_CAPACITY "capacity"
#define REUSABLE_RESOURCE_USE_PREDICATE_NAME "Use"
#define REUSABLE_RESOURCE_USE_AMOUNT_NAME "amount"

namespace ratio
{
  class reusable_resource final : public smart_type
  {
  public:
    reusable_resource(riddle::scope &scp);

  private:
    /* data */
  };
} // namespace ratio
