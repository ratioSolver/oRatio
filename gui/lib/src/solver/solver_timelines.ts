import { Component } from "ratio-core";
import { solver } from "./solver";
import Plotly, { Shape } from 'plotly.js-dist-min';

export class TimelinesChart extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

  private current_time: Partial<Shape> = {
    type: 'line',
    x0: 0,
    y0: 0,
    x1: 0,
    y1: 1,
    xref: 'x',
    yref: 'paper',
    line: {
      color: 'darkgrey',
      width: 2
    }
  };
  private layout = {
    autosize: true,
    xaxis: { title: 'Time' },
    showlegend: false,
    shapes: [this.current_time],
  };
  private config = { responsive: true, displaylogo: false };

  constructor(solver: solver.Solver) {
    super(solver, document.createElement('div'));
    this.element.id = 'slv-' + solver.get_id() + '-timelines';
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
  }

  override mounted(): void {
    Plotly.react(this.element, [], this.layout, this.config);

    this.payload.add_solver_listener(this);
  }

  state_changed(_state: solver.ExecutionState): void { }
  flaw_created(_flaw: solver.graph.Flaw): void { }
  flaw_cost_changed(_flaw: solver.graph.Flaw): void { }
  flaw_state_changed(_flaw: solver.graph.Flaw): void { }
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

  override unmounting(): void { this.payload.remove_solver_listener(this); }
}