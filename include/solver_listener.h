#pragma once

#include "solver.h"
#include "flaw.h"
#include "resolver.h"
#include "sat_value_listener.h"
#include "idl_value_listener.h"
#include <algorithm>

namespace ratio
{
  class solver_listener
  {
    friend class solver;

  public:
    solver_listener(solver &s) : slv(s) { slv.listeners.push_back(this); }
    virtual ~solver_listener() { slv.listeners.erase(std::find(slv.listeners.cbegin(), slv.listeners.cend(), this)); }

  private:
    void new_flaw(const flaw &f)
    {
      flaw_listeners.emplace(&f, new flaw_listener(*this, f));
      flaw_created(f);
    }

    virtual void flaw_created(const flaw &) {}
    virtual void flaw_state_changed(const flaw &) {}
    virtual void flaw_cost_changed(const flaw &) {}
    virtual void flaw_position_changed(const flaw &) {}
    virtual void current_flaw(const flaw &) {}

    void new_resolver(const resolver &r)
    {
      resolver_listeners.emplace(&r, new resolver_listener(*this, r));
      resolver_created(r);
    }

    virtual void resolver_created(const resolver &) {}
    virtual void resolver_state_changed(const resolver &) {}
    virtual void current_resolver(const resolver &) {}

    virtual void causal_link_added(const flaw &, const resolver &) {}

    class flaw_listener : public semitone::sat_value_listener, public semitone::idl_value_listener
    {
    public:
      flaw_listener(solver_listener &l, const flaw &f) : sat_value_listener(l.slv.get_sat_core_ptr()), idl_value_listener(l.slv.get_idl_theory()), listener(l), f(f)
      {
        listen_sat(variable(f.get_phi()));
        listen_idl(f.get_position());
      }
      flaw_listener(const flaw_listener &orig) = delete;
      virtual ~flaw_listener() {}

    private:
      void sat_value_change(const semitone::var &) override { listener.flaw_state_changed(f); }
      void idl_value_change(const semitone::var &) override { listener.flaw_position_changed(f); }

    protected:
      solver_listener &listener;
      const flaw &f;
    };

    class resolver_listener : public semitone::sat_value_listener
    {
    public:
      resolver_listener(solver_listener &l, const resolver &r) : sat_value_listener(l.slv.get_sat_core_ptr()), listener(l), r(r) { listen_sat(variable(r.get_rho())); }
      resolver_listener(const resolver_listener &orig) = delete;
      virtual ~resolver_listener() {}

    private:
      void sat_value_change(const semitone::var &) override { listener.resolver_state_changed(r); }

    protected:
      solver_listener &listener;
      const resolver &r;
    };

  protected:
    solver &slv;

  private:
    std::unordered_map<const flaw *, utils::u_ptr<flaw_listener>> flaw_listeners;
    std::unordered_map<const resolver *, utils::u_ptr<resolver_listener>> resolver_listeners;
  };

  inline json::json flaw_created_message(const flaw &f) noexcept
  {
    json::json flaw_json = to_json(f);
    flaw_json["type"] = "flaw_created";
    flaw_json["solver_id"] = get_id(f.get_solver());
    return flaw_json;
  }
  inline json::json flaw_state_changed_message(const flaw &f) noexcept { return {{"type", "flaw_state_changed"}, {"solver_id", get_id(f.get_solver())}, {"id", get_id(f)}, {"state", f.get_solver().get_sat_core().value(f.get_phi())}}; }
  inline json::json flaw_cost_changed_message(const flaw &f) noexcept { return {{"type", "flaw_cost_changed"}, {"solver_id", get_id(f.get_solver())}, {"id", get_id(f)}, {"cost", to_json(f.get_estimated_cost())}}; }
  inline json::json flaw_position_changed_message(const flaw &f) noexcept { return {{"type", "flaw_position_changed"}, {"solver_id", get_id(f.get_solver())}, {"solver_id", {"id", get_id(f)}, {"pos", to_json(f.get_solver().get_idl_theory().bounds(f.get_position()))}}}; }
  inline json::json current_flaw_message(const flaw &f) noexcept { return {{"type", "current_flaw"}, {"solver_id", get_id(f.get_solver())}, {"id", get_id(f)}}; }

  inline json::json resolver_created_message(const resolver &r) noexcept
  {
    json::json resolver_json = to_json(r);
    resolver_json["type"] = "resolver_created";
    resolver_json["solver_id"] = get_id(r.get_solver());
    return resolver_json;
  }
  inline json::json resolver_state_changed_message(const resolver &r) noexcept { return {{"type", "resolver_state_changed"}, {"solver_id", get_id(r.get_solver())}, {"id", get_id(r)}, {"state", r.get_solver().get_sat_core().value(r.get_rho())}}; }
  inline json::json current_resolver_message(const resolver &r) noexcept { return {{"type", "current_resolver"}, {"solver_id", get_id(r.get_solver())}, {"id", get_id(r)}}; }

  inline json::json causal_link_added_message(const flaw &f, const resolver &r) noexcept { return {{"type", "causal_link_added"}, {"solver_id", get_id(f.get_solver())}, {"flaw_id", get_id(f)}, {"resolver_id", get_id(r)}}; }
} // namespace ratio
