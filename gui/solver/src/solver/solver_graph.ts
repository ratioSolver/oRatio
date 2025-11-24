import { PayloadComponent } from "@ratiosolver/flick";
import { solver } from "./solver";
import cytoscape from 'cytoscape';
import { interpolateRgb } from 'd3-interpolate';

const infiniteColor = "black";
const forbiddenColor = "lightgray";

export class SolverGraph extends PayloadComponent<HTMLDivElement, solver.Solver> implements solver.SolverListener {

  private cy: cytoscape.Core | null = null;
  private layout = {
    name: 'dagre',
    rankDir: 'LR',
    fit: false,
    nodeDimensionsIncludeLabels: true,
    animate: true,
    animationDuration: 50
  };
  private c_flaw: solver.graph.Flaw | null = null;
  private c_resolver: solver.graph.Resolver | null = null
  private tooltip_style = "position: absolute; top: 0; left: 0; background-color: #444; color: white; border-radius: 4px; opacity: 0.8;";
  private costColorScale = interpolateRgb("green", "red");
  private max_cost = 5;

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
        this.cy.add({ group: 'edges', data: { id: `${pre.get_id()}-${resolver.get_id()}`, source: pre.get_id(), target: resolver.get_id(), stroke: stroke_style(resolver) } });
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

  state_changed(): void { }

  flaw_created(flaw: solver.graph.Flaw): void {
    this.create_flaw_node(flaw);

    for (const cause of flaw.get_causes()) {
      this.cy!.add({ group: 'edges', data: { id: `${flaw.get_id()}-${cause.get_id()}`, source: flaw.get_id(), target: cause.get_id(), stroke: stroke_style(flaw) } });
      this.cy!.$id(cause.get_id().toString()).data('color', this.color(cause));
    }
    for (const support of flaw.get_supports()) {
      this.cy!.add({ group: 'edges', data: { id: `${flaw.get_id()}-${support.get_id()}`, source: flaw.get_id(), target: support.get_id(), stroke: stroke_style(flaw) } });
      this.cy!.$id(support.get_id().toString()).data('color', this.color(support));
    }
    this.cy!.layout(this.layout).run();
  }

  flaw_state_changed(flaw: solver.graph.Flaw): void { this.cy!.$id(flaw.get_id().toString()).data({ color: this.color(flaw), stroke: stroke_style(flaw) }); }

  flaw_position_changed(flaw: solver.graph.Flaw): void { this.cy!.$id(flaw.get_id().toString()).data('label', flaw.to_string()); }

  flaw_cost_changed(flaw: solver.graph.Flaw): void {
    if (flaw.get_cost() > this.max_cost && flaw.get_cost() !== Infinity) {
      this.max_cost = flaw.get_cost();
      for (const [_, f] of this.payload.get_flaws())
        this.cy!.$id(f.get_id().toString()).data('color', this.color(f));
      for (const [_, r] of this.payload.get_resolvers())
        this.cy!.$id(r.get_id().toString()).data('color', this.color(r));
    } else {
      this.cy!.$id(flaw.get_id().toString()).data('color', this.color(flaw));
      for (const cause of flaw.get_causes())
        this.cy!.$id(cause.get_id().toString()).data('color', this.color(cause));
      for (const support of flaw.get_supports())
        this.cy!.$id(support.get_id().toString()).data('color', this.color(support));
    }
  }

  current_flaw(flaw: solver.graph.Flaw | null): void {
    if (this.c_flaw)
      this.cy!.$id(this.c_flaw.get_id().toString()).removeClass('current');
    if (flaw) {
      this.cy!.$id(flaw.get_id().toString()).addClass('current');
      this.c_flaw = flaw;
    }
  }

  resolver_created(resolver: solver.graph.Resolver): void {
    this.create_resolver_node(resolver);

    this.cy!.add({ group: 'edges', data: { id: `${resolver.get_id()}-${resolver.get_flaw().get_id()}`, source: resolver.get_id(), target: resolver.get_flaw().get_id(), stroke: stroke_style(resolver) } });
    this.cy!.layout(this.layout).run();
  }

  resolver_state_changed(resolver: solver.graph.Resolver): void {
    this.cy!.$id(resolver.get_id().toString()).data({ color: this.color(resolver), stroke: stroke_style(resolver) });
    this.cy!.$id(`${resolver.get_id()}-${resolver.get_flaw().get_id()}`).data('stroke', stroke_style(resolver));
    for (const pre of resolver.get_preconditions())
      this.cy!.$id(`${pre.get_id()}-${resolver.get_id()}`).data('stroke', stroke_style(resolver));
  }

  current_resolver(resolver: solver.graph.Resolver | null): void {
    if (this.c_resolver)
      this.cy!.$id(this.c_resolver.get_id().toString()).removeClass('current');
    if (resolver) {
      this.cy!.$id(resolver.get_id().toString()).addClass('current');
      this.c_resolver = resolver;
    }
  }

  causal_link_added(flaw: solver.graph.Flaw, resolver: solver.graph.Resolver): void {
    this.cy!.add({ group: 'edges', data: { id: `${flaw.get_id()}-${resolver.get_id()}`, source: flaw.get_id(), target: resolver.get_id(), stroke: stroke_style(resolver) } });
    this.cy!.layout(this.layout).run();
  }

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

  private create_flaw_node(flaw: solver.graph.Flaw): cytoscape.CollectionReturnValue {
    const fn = this.cy!.add({ group: 'nodes', data: { id: flaw.get_id().toString(), type: 'flaw', label: flaw.to_string(), color: this.color(flaw), stroke: stroke_style(flaw) } });
    fn.on('mouseover', () => {
      const popper = fn.popper({
        content: () => {
          var div = document.createElement('div');
          div.style.cssText = this.tooltip_style;
          div.innerHTML = flaw.to_string(true);
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

  private create_resolver_node(resolver: solver.graph.Resolver): cytoscape.CollectionReturnValue {
    const rn = this.cy!.add({ group: 'nodes', data: { id: resolver.get_id().toString(), type: 'resolver', label: resolver.to_string(), color: this.color(resolver), stroke: stroke_style(resolver) } });
    rn.on('mouseover', () => {
      const popper = rn.popper({
        content: () => {
          var div = document.createElement('div');
          div.style.cssText = this.tooltip_style;
          div.innerHTML = resolver.to_string(true);
          document.body.appendChild(div);
          return div;
        }
      });
      rn.scratch('popper', popper);
    });
    rn.on('mouseout', () => {
      const popper = rn.scratch('popper');
      if (popper) {
        popper.destroy();
        rn.removeScratch('popper');
      }
    });
    rn.on('position', () => {
      const popper = rn.scratch('popper');
      if (popper)
        popper.update();
    });
    return rn;
  }

  private color(node: solver.graph.Flaw | solver.graph.Resolver): string {
    if (node.get_state() === solver.graph.State.forbidden)
      return forbiddenColor;
    const cost = node.get_cost();
    if (cost === Infinity)
      return infiniteColor; // We use black for infinite cost
    else
      return this.costColorScale(cost / this.max_cost); // We use a gradient from green to red for costs between 0 and 100
  }
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