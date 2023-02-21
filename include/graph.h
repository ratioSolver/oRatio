#pragma once

namespace ratio
{
  class solver;

  class graph
  {
  public:
    graph(solver &s);
    virtual ~graph() = default;

  private:
    solver &s;
  };
} // namespace ratio
