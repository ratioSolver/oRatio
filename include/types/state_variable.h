#pragma once

#include "smart_type.h"

#define STATE_VARIABLE_NAME "StateVariable"

namespace ratio
{
  class state_variable final : public smart_type
  {
  public:
    state_variable(riddle::scope &scp);

  private:
    /* data */
  };
} // namespace ratio
