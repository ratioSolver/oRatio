#pragma once

#include "item.h"

namespace ratio
{
  /**
   * @brief A class for representing boolean items.
   *
   */
  class bool_item : public riddle::item
  {
  public:
    bool_item(riddle::type &t, const semitone::lit &l) : item(t), l(l) {}

  private:
    const semitone::lit l;
  };

  /**
   * @brief A class for representing arithmetic items. Arithmetic items can be either integers, reals, or time points.
   *
   */
  class arith_item : public riddle::item
  {
  public:
    arith_item(riddle::type &t, const semitone::lin &l) : item(t), l(l) {}

  private:
    const semitone::lin l;
  };

  /**
   * @brief A class for representing string items.
   *
   */
  class string_item : public riddle::item
  {
  public:
    string_item(riddle::type &t, const std::string &s = "") : item(t), s(s) {}

  private:
    const std::string s;
  };

  /**
   * @brief A class for representing enum items.
   *
   */
  class enum_item : public riddle::item
  {
  public:
    enum_item(riddle::type &t, const semitone::var &ev) : item(t), ev(ev) {}

  private:
    const semitone::var ev;
  };

  /**
   * @brief A class for representing standard complex items.
   *
   */
  class complex_item : public riddle::complex_item, public semitone::var_value
  {
  public:
    complex_item(riddle::complex_type &t) : riddle::complex_item(t) {}
  };

  /**
   * @brief A class for representing atoms.
   *
   */
  class atom : public riddle::complex_item
  {
  public:
    atom(riddle::predicate &p) : complex_item(p) {}
  };
} // namespace ratio
