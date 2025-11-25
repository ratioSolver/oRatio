#pragma once

#include "solver_core.hpp"
#include "items.hpp"

namespace ratio
{
  class flaw;
  class resolver;
  class atom_flaw;

  class atom : public riddle::atom
  {
  public:
    atom(riddle::predicate &pred, bool is_fact, std::map<std::string, riddle::expr, std::less<>> &&args, utils::lit &&sigma, atom_flaw &flaw) noexcept : riddle::atom(pred, is_fact, std::move(args), std::move(sigma)), flaw(flaw) {}

    [[nodiscard]] atom_flaw &get_flaw() noexcept { return flaw; }

  private:
    atom_flaw &flaw; // the flaw associated with this atom..
  };

  class solver : public solver_core
  {
    friend class flaw;
    friend class resolver;
    static constexpr const char *INIT_STRING = "predicate Impulse(real at) { at >= origin; at <= horizon; } predicate Interval(real start, real end, real duration) { start >= origin; duration == end - start; duration >= 0.0; end <= horizon; } real origin, horizon; origin >= 0.0; origin <= horizon;";

  public:
    solver() noexcept;

    [[nodiscard]] riddle::expr new_enum(riddle::component_type &tp, std::vector<riddle::expr> &&values) override;

    void new_clause(std::vector<riddle::bool_expr> &&exprs) override;
    void new_disjunction(std::vector<std::unique_ptr<riddle::conjunction>> &&disjuncts) override;

    void solve() override;

  private:
    [[nodiscard]] riddle::atom_expr create_atom(bool is_fact, riddle::predicate &pred, std::map<std::string, riddle::expr, std::less<>> &&args) override;

    struct Node
    {
      std::size_t id = 0;                                   // The unique identifier of the node..
      std::shared_ptr<Node> parent;                         // The parent node..
      std::optional<std::reference_wrapper<resolver>> res;  // The resolver applied to reach this node..
      std::unordered_set<std::shared_ptr<flaw>> open_flaws; // The set of open flaws..
    };
    std::shared_ptr<Node> find_common_ancestor(std::shared_ptr<Node> a, std::shared_ptr<Node> b) const;

    void backtrack_to(const std::shared_ptr<Node> &lca) noexcept;

    void go_to(const std::shared_ptr<Node> &target);

  private:
    std::shared_ptr<Node> current_node;        // The current node in the search tree..
    std::vector<std::shared_ptr<Node>> fringe; // The fringe of the search tree..
  };
} // namespace ratio
