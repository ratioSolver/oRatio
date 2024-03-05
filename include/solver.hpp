#pragma once

#include "solver_item.hpp"
#include "core.hpp"
#include "lra_theory.hpp"
#include "idl_theory.hpp"
#include "rdl_theory.hpp"
#include "ov_theory.hpp"
#include "graph.hpp"

namespace ratio
{
  class solver : public riddle::core, public semitone::theory
  {
  public:
    solver(const std::string &name = "oRatio", bool i = true) noexcept;
    virtual ~solver() = default;

    void init() noexcept;

    std::shared_ptr<riddle::item> new_bool() noexcept override;
    std::shared_ptr<riddle::item> new_bool(bool value) noexcept override;
    std::shared_ptr<riddle::item> new_int() noexcept override;
    std::shared_ptr<riddle::item> new_int(INTEGER_TYPE value) noexcept override;
    std::shared_ptr<riddle::item> new_real() noexcept override;
    std::shared_ptr<riddle::item> new_real(const utils::rational &value) noexcept override;
    std::shared_ptr<riddle::item> new_time() noexcept override;
    std::shared_ptr<riddle::item> new_time(const utils::rational &value) noexcept override;
    std::shared_ptr<riddle::item> new_string() noexcept override;
    std::shared_ptr<riddle::item> new_string(const std::string &value) noexcept override;
    std::shared_ptr<riddle::item> new_enum(riddle::type &tp, std::vector<std::reference_wrapper<utils::enum_val>> &&values) override;

    std::shared_ptr<riddle::item> minus(const std::shared_ptr<riddle::item> &xpr) override;
    std::shared_ptr<riddle::item> add(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> sub(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> mul(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;
    std::shared_ptr<riddle::item> div(const std::vector<std::shared_ptr<riddle::item>> &xprs) override;

    std::shared_ptr<riddle::item> lt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> leq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> gt(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> geq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;
    std::shared_ptr<riddle::item> eq(const std::shared_ptr<riddle::item> &lhs, const std::shared_ptr<riddle::item> &rhs) override;

    std::shared_ptr<riddle::item> conj(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> disj(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> exct_one(const std::vector<std::shared_ptr<riddle::item>> &exprs) override;
    std::shared_ptr<riddle::item> negate(const std::shared_ptr<riddle::item> &expr) override;

  private:
    bool propagate(const utils::lit &) noexcept override { return true; }
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    std::string name;
    semitone::lra_theory lra;
    semitone::idl_theory idl;
    semitone::rdl_theory rdl;
    semitone::ov_theory ov;
    graph gr;
  };
} // namespace ratio
