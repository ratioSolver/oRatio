import { Component } from "../app";
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
    super(solver, document.querySelector('#slv-' + solver.get_id() + '-timelines') as HTMLDivElement);
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    solver.add_solver_listener(this);
  }

  init(items: Map<string, solver.values.Value>, atoms: Map<number, solver.values.Atom>, state: solver.SolverState, flaws: Map<number, solver.graph.Flaw>, resolvers: Map<number, solver.graph.Resolver>, c_flaw: solver.graph.Flaw | null, c_resolver: solver.graph.Resolver | null): void {
    Plotly.react(this.element, [], this.layout, this.config);
  }

  state_changed(state: solver.SolverState): void { }
  flaw_created(flaw: solver.graph.Flaw): void { }
  flaw_cost_changed(flaw: solver.graph.Flaw): void { }
  current_flaw(flaw: solver.graph.Flaw | null): void { }
  resolver_created(resolver: solver.graph.Resolver): void { }
  current_resolver(resolver: solver.graph.Resolver | null): void { }

  unmounting(): void { this.payload.remove_solver_listener(this); }
}