#pragma once

#include "item.h"
#include "lin.h"
#include "lit.h"
#include "type.h"

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

    semitone::lit get_lit() const { return l; }

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

    semitone::lin get_lin() const { return l; }

  private:
    const semitone::lin l;
  };

  /**
   * @brief A class for representing string items.
   *
   */
  class string_item : public riddle::item, public utils::enum_val
  {
  public:
    string_item(riddle::type &t, const std::string &s = "") : item(t), s(s) {}

    const std::string &get_string() const { return s; }

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

    semitone::var get_var() const { return ev; }

  private:
    const semitone::var ev;
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
