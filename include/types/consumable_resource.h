#pragma once

#include "smart_type.h"

#define CONSUMABLE_RESOURCE_NAME "ConsumableResource"
#define CONSUMABLE_RESOURCE_INITIAL_AMOUNT "initial_amount"
#define CONSUMABLE_RESOURCE_CAPACITY "capacity"
#define CONSUMABLE_RESOURCE_PRODUCE_PREDICATE_NAME "Produce"
#define CONSUMABLE_RESOURCE_CONSUME_PREDICATE_NAME "Consume"
#define CONSUMABLE_RESOURCE_USE_AMOUNT_NAME "amount"

namespace ratio
{
  class consumable_resource final : public smart_type
  {
  public:
    consumable_resource(riddle::scope &scp);

  private:
    /* data */
  };
} // namespace ratio
