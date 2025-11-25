import { PayloadComponent } from "@ratiosolver/flick";
import { solver } from "./solver";
import cytoscape from 'cytoscape';

export class SolverTree extends PayloadComponent<HTMLDivElement, solver.Solver> implements solver.SolverListener {

  private cy: cytoscape.Core | null = null;
  private layout = {
    name: 'dagre',
    fit: false,
    nodeDimensionsIncludeLabels: true,
    animate: true,
    animationDuration: 50
  };
  private c_node: solver.tree.Node | null = null;
  private tooltip_style = "position: absolute; top: 0; left: 0; background-color: #444; color: white; border-radius: 4px; opacity: 0.8;";

  constructor(solver: solver.Solver) {
    super(document.createElement('div'), solver);
    this.node.id = 'slv-' + solver.get_id() + '-graph';
    this.node.classList.add('d-flex', 'flex-column', 'flex-grow-1');
  }

  override mounted(): void {
    this.cy = cytoscape({
      container: this.node,
      style: [
        {
          selector: 'node',
          style: {
            'shape': 'round-rectangle',
            'background-color': '#888',
            'label': 'data(label)',
            'border-width': '1px',
            'border-color': '#666'
          }
        },
        {
          selector: 'edge',
          style: {
            'curve-style': 'bezier',
            'line-color': '#666',
            'target-arrow-color': '#666',
            'target-arrow-shape': 'triangle',
            'width': '1px'
          }
        },
        {
          selector: 'node.current',
          style: {
            'border-width': '3px',
            'border-color': '#919'
          }
        }
      ]
    });

    for (const [_, node] of this.payload.get_tree())
      this.create_tree_node(node);

    for (const [_, node] of this.payload.get_tree())
      if (node.get_parent())
        this.cy.add({ group: 'edges', data: { id: `${node.get_parent()!.get_id()}-${node.get_id()}`, source: node.get_parent()!.get_id(), target: node.get_id(), stroke: 'solid' } });

    this.cy.layout(this.layout).run();

    this.payload.add_solver_listener(this);
  }

  state_changed(): void { }

  node_created(node: solver.tree.Node): void {
    this.create_tree_node(node);
    this.cy!.add({ group: 'edges', data: { id: `${node.get_parent()!.get_id()}-${node.get_id()}`, source: node.get_parent()!.get_id(), target: node.get_id(), stroke: 'solid' } });
    this.cy!.layout(this.layout).run();
  }
  current_node(node: solver.tree.Node | null): void {
    if (this.c_node)
      this.cy!.$id(this.c_node.get_id().toString()).removeClass('current');
    if (node) {
      this.cy!.$id(node.get_id().toString()).addClass('current');
      this.c_node = node;
    }
  }

  flaw_created(_flaw: solver.graph.Flaw): void { }
  flaw_state_changed(_flaw: solver.graph.Flaw): void { }
  flaw_position_changed(_flaw: solver.graph.Flaw): void { }
  flaw_cost_changed(_flaw: solver.graph.Flaw): void { }
  current_flaw(_flaw: solver.graph.Flaw | null): void { }
  resolver_created(_resolver: solver.graph.Resolver): void { }
  resolver_state_changed(_resolver: solver.graph.Resolver): void { }
  current_resolver(_resolver: solver.graph.Resolver | null): void { }
  causal_link_added(_flaw: solver.graph.Flaw, _resolver: solver.graph.Resolver): void { }
  execution_state_changed(_state: solver.ExecutionState): void { }
  tick(_time: solver.values.Rational): void { }
  starting(_atoms: solver.values.Atom[]): void { }
  start(_atoms: solver.values.Atom[]): void { }
  ending(_atoms: solver.values.Atom[]): void { }
  end(_atoms: solver.values.Atom[]): void { }

  override unmounting(): void {
    this.payload.remove_solver_listener(this);
    this.cy!.destroy();
  }

  private create_tree_node(node: solver.tree.Node): cytoscape.CollectionReturnValue {
    const fn = this.cy!.add({ group: 'nodes', data: { id: node.get_id().toString(), label: node.to_string() } });
    fn.on('mouseover', () => {
      const popper = fn.popper({
        content: () => {
          var div = document.createElement('div');
          div.style.cssText = this.tooltip_style;
          div.innerHTML = node.to_string(true);
          document.body.appendChild(div);
          return div;
        }
      });
      fn.scratch('popper', popper);
    });
    fn.on('mouseout', () => {
      const popper = fn.scratch('popper');
      if (popper) {
        popper.destroy();
        fn.removeScratch('popper');
      }
    });
    fn.on('position', () => {
      const popper = fn.scratch('popper');
      if (popper)
        popper.update();
    });
    return fn;
  }
}
