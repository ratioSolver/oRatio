import { Component } from "ratio-core";
import { solver } from "./solver";
import Plotly, { Layout, PlotData, Shape } from 'plotly.js-dist-min';

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
  private layout: Partial<Layout> & { [key: `yaxis${number}`]: Partial<Plotly.LayoutAxis>; } = {
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
    this.payload.add_solver_listener(this);
    this.state_changed();
  }

  state_changed(): void {
    const data: Partial<PlotData>[] = [];
    let i = 1;
    const domain_size = 1 / this.payload.get_timelines().size;
    const domain_separator = 0.05 * domain_size;
    let start_domain = 0;
    const yaxis: Map<number, string> = new Map();
    for (const [id, tl] of this.payload.get_timelines()) {
      const gen = solver.chart.ChartManager.get_instance().get_chart_generator(tl.get_type());
      const c = gen.make_chart(tl);
      const layout = { title: tl.get_name(), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: gen.show_tick_labels(), showgrid: gen.show_grid(), range: c.get_range() };
      if (i == 1) {
        yaxis.set(id, 'y');
        this.layout['yaxis'] = layout;
      }
      else {
        yaxis.set(id, `y${i}`);
        this.layout[`yaxis${i}`] = layout;
      }
      const c_yaxis = yaxis.get(id);
      for (const d of c.get_data()) {
        d.yaxis = c_yaxis;
        data.push(d);
      }
      start_domain += domain_size;
      i++;
    }

    Plotly.react(this.element, data.flat(), this.layout, this.config);
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
  tick(time: solver.values.Rational): void {
    this.current_time.x0 = time.to_number();
    this.current_time.x1 = time.to_number();
    Plotly.react(this.element, [], this.layout, this.config);
  }
  starting(_atoms: solver.values.Atom[]): void { }
  start(_atoms: solver.values.Atom[]): void { }
  ending(_atoms: solver.values.Atom[]): void { }
  end(_atoms: solver.values.Atom[]): void { }

  override unmounting(): void { this.payload.remove_solver_listener(this); }
}