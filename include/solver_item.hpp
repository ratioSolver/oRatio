#pragma once

#include "item.hpp"
#include "type.hpp"
#include "lit.hpp"
#include "lin.hpp"

namespace ratio
{
  class bool_item : public riddle::item
  {
  public:
    bool_item(riddle::bool_type &t, const utils::lit &l) : item(t), value(l) {}

    utils::lit &get_value() { return value; }
    const utils::lit &get_value() const { return value; }

  private:
    utils::lit value;
  };

  class arith_item : public riddle::item
  {
  public:
    arith_item(riddle::int_type &t, const utils::lin &l) : item(t), value(l) {}
    arith_item(riddle::real_type &t, const utils::lin &l) : item(t), value(l) {}
    arith_item(riddle::time_type &t, const utils::lin &l) : item(t), value(l) {}

    utils::lin &get_value() { return value; }
    const utils::lin &get_value() const { return value; }

  private:
    utils::lin value;
  };

  class string_item : public riddle::item
  {
  public:
    string_item(riddle::string_type &t, const std::string &s = "") : item(t), value(s) {}

    std::string &get_value() { return value; }
    const std::string &get_value() const { return value; }

  private:
    std::string value;
  };

  class enum_item : public riddle::item, public riddle::env
  {
  public:
    enum_item(riddle::type &t, VARIABLE_TYPE v) : item(t), env(t.get_scope().get_core()), value(v) {}

    std::shared_ptr<riddle::item> get(const std::string &name) const noexcept override;

    VARIABLE_TYPE &get_value() { return value; }
    const VARIABLE_TYPE &get_value() const { return value; }

  private:
    VARIABLE_TYPE value;
  };

  class atom : public riddle::item, public riddle::env
  {
  public:
    atom(riddle::predicate &p, bool is_fact, const utils::lit &s, std::map<std::string, std::shared_ptr<item>> &&args = {}) : item(p), env(p.get_scope().get_core()), fact(is_fact), sigma(s)
    {
      for (auto &[name, item] : args)
        items.emplace(name, item);
    }

    bool is_fact() const { return fact; }
    utils::lit &get_sigma() { return sigma; }
    const utils::lit &get_sigma() const { return sigma; }

  private:
    bool fact;
    utils::lit sigma;
  };
} // namespace ratio
