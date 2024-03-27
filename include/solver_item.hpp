#pragma once

#include "item.hpp"
#include "type.hpp"
#include "lit.hpp"
#include "lin.hpp"

namespace ratio
{
  class atom_flaw;

  class bool_item : public riddle::item
  {
  public:
    bool_item(riddle::bool_type &t, const utils::lit &l) : item(t), value(l) {}

    [[nodiscard]] utils::lit &get_value() { return value; }
    [[nodiscard]] const utils::lit &get_value() const { return value; }

  private:
    utils::lit value;
  };

  class arith_item : public riddle::item
  {
  public:
    arith_item(riddle::int_type &t, const utils::lin &l) : item(t), value(l) {}
    arith_item(riddle::real_type &t, const utils::lin &l) : item(t), value(l) {}
    arith_item(riddle::time_type &t, const utils::lin &l) : item(t), value(l) {}

    [[nodiscard]] utils::lin &get_value() { return value; }
    [[nodiscard]] const utils::lin &get_value() const { return value; }

  private:
    utils::lin value;
  };

  class string_item : public riddle::item
  {
  public:
    string_item(riddle::string_type &t, const std::string &s = "") : item(t), value(s) {}

    [[nodiscard]] std::string &get_value() { return value; }
    [[nodiscard]] const std::string &get_value() const { return value; }

  private:
    std::string value;
  };

  class enum_item : public riddle::item, public riddle::env
  {
  public:
    enum_item(riddle::type &t, VARIABLE_TYPE v) : item(t), env(t.get_scope().get_core()), value(v) {}

    [[nodiscard]] std::shared_ptr<riddle::item> get(const std::string &name) const noexcept override;

    [[nodiscard]] VARIABLE_TYPE &get_value() { return value; }
    [[nodiscard]] const VARIABLE_TYPE &get_value() const { return value; }

  private:
    VARIABLE_TYPE value;
  };

  class atom : public riddle::item, public riddle::env
  {
    friend class atom_flaw;

  public:
    atom(riddle::predicate &p, atom_flaw &reason, bool is_fact, const utils::lit &s, std::map<std::string, std::shared_ptr<item>> &&args = {}) : item(p), env(p.get_scope().get_core()), reason(reason), fact(is_fact), sigma(s)
    {
      for (auto &[name, item] : args)
        items.emplace(name, item);
    }

    [[nodiscard]] atom_flaw &get_reason() const { return reason; }
    [[nodiscard]] bool is_fact() const { return fact; }
    [[nodiscard]] utils::lit &get_sigma() { return sigma; }
    [[nodiscard]] const utils::lit &get_sigma() const { return sigma; }

  private:
    atom_flaw &reason; // the flaw associated to this atom..
    bool fact;         // whether the atom is a fact or a goal..
    utils::lit sigma;  // the literal indicating whether the atom is active or not..
  };
} // namespace ratio
