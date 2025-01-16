import { Component } from "../app";
import { solver } from "./solver";
import cytoscape from 'cytoscape';
import { interpolateRgb } from 'd3-interpolate';

const costColorScale = interpolateRgb("green", "red");
const infiniteColor = "black";

export class SolverGraph extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

  private cy: cytoscape.Core | null = null;
  private layout = {
    name: 'dagre',
    rankDir: 'LR',
    fit: false,
    nodeDimensionsIncludeLabels: true,
    animate: true,
    animationDuration: 100
  };
  private c_flaw: solver.graph.Flaw | null = null;
  private c_resolver: solver.graph.Resolver | null = null

  constructor(solver: solver.Solver) {
    super(solver, document.createElement('div'));
    this.element.id = 'slv-' + solver.get_id() + '-graph';
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
  }

  mounted(): void {
    this.cy = cytoscape({
      container: this.element,
      style: [
        {
          selector: 'node[type="flaw"]',
          style: {
            'shape': 'round-rectangle',
            'background-color': 'data(color)',
            'label': 'data(label)',
            'border-width': '1px',
            'border-style': 'data(stroke)' as any,
            'border-color': '#666'
          }
        },
        {
          selector: 'node[type="resolver"]',
          style: {
            'shape': 'ellipse',
            'background-color': 'data(color)',
            'label': 'data(label)',
            'border-width': '1px',
            'border-style': 'data(stroke)' as any,
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
            'width': '1px',
            'line-style': 'data(stroke)' as any
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

    for (const [_, flaw] of this.payload.get_flaws())
      this.create_flaw_node(flaw);

    for (const [_, resolver] of this.payload.get_resolvers()) {
      this.create_resolver_node(resolver);

      this.cy.add({ group: 'edges', data: { id: `${resolver.get_id()}-${resolver.get_flaw().get_id()}`, source: resolver.get_id(), target: resolver.get_flaw().get_id(), stroke: stroke_style(resolver) } });
      for (const pre of resolver.get_preconditions())
        this.cy.add({ group: 'edges', data: { id: `${resolver.get_id()}-${pre.get_id()}`, source: resolver.get_id(), target: pre.get_id(), stroke: stroke_style(resolver) } });
    }

    if (this.payload.get_current_flaw()) {
      this.cy.$id(this.payload.get_current_flaw()!.get_id().toString()).addClass('current');
      this.c_flaw = this.payload.get_current_flaw();
    }

    if (this.payload.get_current_resolver()) {
      this.cy.$id(this.payload.get_current_resolver()!.get_id().toString()).addClass('current');
      this.c_resolver = this.payload.get_current_resolver();
    }
    this.cy.layout(this.layout).run();

    this.payload.add_solver_listener(this);
  }

  state_changed(state: solver.SolverState): void { }

  flaw_created(flaw: solver.graph.Flaw): void {
    this.create_flaw_node(flaw);

    for (const cause of flaw.get_causes())
      this.cy!.add({ group: 'edges', data: { id: `${flaw.get_id()}-${cause.get_id()}`, source: flaw.get_id(), target: cause.get_id(), stroke: stroke_style(flaw) } });
    this.cy!.layout(this.layout).run();
  }

  flaw_state_changed(flaw: solver.graph.Flaw): void {
    this.cy!.$id(flaw.get_id().toString()).data('color', color(flaw));
    this.cy!.layout(this.layout).run();
  }

  flaw_cost_changed(flaw: solver.graph.Flaw): void {
    this.cy!.$id(flaw.get_id().toString()).data('color', color(flaw));
    this.cy!.layout(this.layout).run();
  }

  current_flaw(flaw: solver.graph.Flaw | null): void {
    if (this.c_flaw)
      this.cy!.$id(this.c_flaw.get_id().toString()).removeClass('current');
    if (flaw) {
      this.cy!.$id(flaw.get_id().toString()).addClass('current');
      this.c_flaw = flaw;
    }
    this.cy!.layout(this.layout).run();
  }

  resolver_created(resolver: solver.graph.Resolver): void {
    this.create_resolver_node(resolver);

    this.cy!.add({ group: 'edges', data: { id: `${resolver.get_id()}-${resolver.get_flaw().get_id()}`, source: resolver.get_id(), target: resolver.get_flaw().get_id(), stroke: stroke_style(resolver) } });
    this.cy!.layout(this.layout).run();
  }

  resolver_state_changed(resolver: solver.graph.Resolver): void {
    this.cy!.$id(resolver.get_id().toString()).data('color', color(resolver));
    this.cy!.layout(this.layout).run();
  }

  current_resolver(resolver: solver.graph.Resolver | null): void {
    if (this.c_resolver)
      this.cy!.$id(this.c_resolver.get_id().toString()).removeClass('current');
    if (resolver) {
      this.cy!.$id(resolver.get_id().toString()).addClass('current');
      this.c_resolver = resolver;
    }
    this.cy!.layout(this.layout).run();
  }

  causal_link_added(flaw: solver.graph.Flaw, resolver: solver.graph.Resolver): void {
    this.cy!.add({ group: 'edges', data: { id: `${flaw.get_id()}-${resolver.get_id()}`, source: flaw.get_id(), target: resolver.get_id(), stroke: stroke_style(resolver) } });
    this.cy!.layout(this.layout).run();
  }

  execution_state_changed(state: solver.SolverState): void { }
  tick(time: solver.values.Rational): void { }
  starting(atoms: solver.values.Atom[]): void { }
  start(atoms: solver.values.Atom[]): void { }
  ending(atoms: solver.values.Atom[]): void { }
  end(atoms: solver.values.Atom[]): void { }

  unmounting(): void { this.payload.remove_solver_listener(this); }

  private create_flaw_node(flaw: solver.graph.Flaw): cytoscape.CollectionReturnValue {
    const fn = this.cy!.add({ group: 'nodes', data: { id: flaw.get_id().toString(), type: 'flaw', label: flaw.to_string(), color: color(flaw), stroke: stroke_style(flaw) } });
    this.cy!.on('mouseover', 'node', () => {
      const popper = fn.popper({
        content: () => {
          const div = document.createElement('div');
          div.innerHTML = flaw.to_string(true);
          return div;
        }
      });
      fn.scratch('popper', popper);
    });
    this.cy!.on('mouseout', 'node', () => {
      const popper = fn.scratch('popper');
      if (popper) {
        popper.destroy();
        fn.removeScratch('popper');
      }
    });
    return fn;
  }

  private create_resolver_node(resolver: solver.graph.Resolver): cytoscape.CollectionReturnValue {
    const rn = this.cy!.add({ group: 'nodes', data: { id: resolver.get_id().toString(), type: 'resolver', label: resolver.to_string(), color: color(resolver), stroke: stroke_style(resolver) } });
    this.cy!.on('mouseover', 'node', () => {
      const popper = rn.popper({
        content: () => {
          const div = document.createElement('div');
          div.innerHTML = resolver.to_string(true);
          return div;
        }
      });
      rn.scratch('popper', popper);
    });
    this.cy!.on('mouseout', 'node', () => {
      const popper = rn.scratch('popper');
      if (popper) {
        popper.destroy();
        rn.removeScratch('popper');
      }
    });
    return rn;
  }
}

function color(flaw: solver.graph.Flaw | solver.graph.Resolver): string {
  const cost = flaw.get_cost();
  if (cost === Infinity)
    return infiniteColor; // We use black for infinite cost
  else if (cost > 100)
    return costColorScale(1); // We use red for high costs
  else
    return costColorScale(cost / 100); // We use a gradient from green to red for costs between 0 and 100
}

function stroke_style(node: solver.graph.Flaw | solver.graph.Resolver): string {
  switch (node.get_state()) {
    case solver.graph.State.active:
      return 'solid';
    case solver.graph.State.forbidden:
      return 'dotted';
    case solver.graph.State.inactive:
      return 'dashed';
  }
}