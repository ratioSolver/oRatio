#pragma once

#include "item.h"

namespace ratio
{
  class bool_item : public riddle::item
  {
  public:
    bool_item(riddle::type &t, const semitone::lit &l) : item(t), l(l) {}

  private:
    const semitone::lit l;
  };

  class arith_item : public riddle::item
  {
  public:
    arith_item(riddle::type &t, const semitone::lin &l) : item(t), l(l) {}

  private:
    const semitone::lin l;
  };

  class string_item : public riddle::item
  {
  public:
    string_item(riddle::type &t, const std::string &s = "") : item(t), s(s) {}

  private:
    const std::string s;
  };

  class enum_item : public riddle::item
  {
  public:
    enum_item(riddle::type &t, const semitone::var &ev) : item(t), ev(ev) {}

  private:
    const semitone::var ev;
  };

  class complex_item : public riddle::complex_item, public semitone::var_value
  {
  public:
    complex_item(riddle::complex_type &t) : riddle::complex_item(t) {}
  };

  class atom : public riddle::complex_item
  {
  public:
    atom(riddle::predicate &p) : complex_item(p) {}
  };
} // namespace ratio
