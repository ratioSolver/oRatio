#pragma once

#include "smart_type.h"

#define AGENT_NAME "Agent"

namespace ratio
{
  class agent final : public smart_type
  {
  public:
    agent(riddle::scope &scp);

  private:
    /* data */
  };
} // namespace ratio
