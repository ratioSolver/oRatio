import { Component } from "ratio-core";
import { solver } from "./solver";
import Plotly, { Layout, Shape } from 'plotly.js-dist-min';

export class TimelinesChart extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

  private origin = 0;
  private horizon = 1;
  private y_axes = new Map<number, string>();
  private traces = new Map<number, any[]>();
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
  }

  state_changed(): void {
    this.origin = (this.payload.get_exprs().get('origin') as solver.values.Real).val.to_number();
    this.horizon = (this.payload.get_exprs().get('horizon') as solver.values.Real).val.to_number();

    let i = 1;
    let start_domain = 0;
    this.y_axes.clear();
    this.traces.clear();
    if (this.payload.get_timelines().size == 0)
      return;

    let domain_size = 1 / this.payload.get_timelines().size;
    const domain_separator = 0.05 * domain_size;
    for (const [id, tl] of this.payload.get_timelines()) {
      if (i == 1)
        this.y_axes.set(id, 'y');
      else
        this.y_axes.set(id, 'y' + i);
      this.traces.set(id, []);

      if (tl instanceof solver.timelines.SolverTimeline) {
        const slv_ends = [0];
        for (const val of tl.get_values()) {
          const start = val.exprs.has('at') ? (val.exprs.get('at') as solver.values.Real).val.to_number() : (val.exprs.get('start') as solver.values.Real).val.to_number();
          const end = val.exprs.has('at') ? start + 1 : (val.exprs.get('end') as solver.values.Real).val.to_number();
          const y = values_y(start, start === end ? start + 0.1 : end, slv_ends);
          const text = solver.timelines.SolverTimeline.to_string(this.payload, val);
          this.traces.get(id)!.push({ x: [start, end], y: [y, y], name: text, text: [text], type: 'scatter', opacity: 0.7, mode: 'lines+text', line: { width: 30 }, textposition: 'middle right', yaxis: this.y_axes.get(id) });
        }
        if (i == 1)
          this.layout['yaxis'] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: false, showgrid: false };
        else
          this.layout[`yaxis${i}`] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: false, showgrid: false };
      }

      if (tl instanceof solver.timelines.StateVariableTimeline) {
        for (const val of tl.get_values()) {
          const text = solver.timelines.StateVariableTimeline.to_string(this.payload, val);
          this.traces.get(id)!.push({ x: [val.start.to_number(), val.end.to_number()], y: [i, i], name: text, text: [text], type: 'scatter', opacity: 0.7, mode: 'lines+text', line: { width: 30 }, textposition: 'middle right', yaxis: this.y_axes.get(id) });
        }
        if (i == 1)
          this.layout['yaxis'] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: false, showgrid: false };
        else
          this.layout[`yaxis${i}`] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: false, showgrid: false };
      }

      if (tl instanceof solver.timelines.ReusableResourceTimeline) {
        const vals_xs = [this.origin];
        const vals_ys = [0];
        for (const val of tl.get_values()) {
          vals_xs.push(val.start.to_number());
          vals_ys.push(val.usage.to_number());
          vals_xs.push(val.end.to_number());
          vals_ys.push(val.usage.to_number());
        }
        vals_xs.push(this.horizon);
        vals_ys.push(0);
        this.traces.get(id)!.push({ x: vals_xs, y: vals_ys, name: solver.timelines.Timeline.timeline_name(this.payload, tl), type: 'scatter', opacity: 0.7, mode: 'lines', fill: 'tozeroy', yaxis: this.y_axes.get(id) });
        this.traces.get(id)!.push({ x: [this.origin, this.horizon], y: [tl.capacity, tl.capacity], name: 'Capacity', type: 'scatter', opacity: 0.7, mode: 'lines', yaxis: this.y_axes.get(id) });
        if (i == 1)
          this.layout['yaxis'] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, range: [0, tl.capacity] };
        else
          this.layout[`yaxis${i}`] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, range: [0, tl.capacity] };
      }

      if (tl instanceof solver.timelines.ConsumableResourceTimeline) {
        const vals_xs = [this.origin];
        const vals_ys = [tl.initial_amount.to_number()];
        for (const val of tl.get_values()) {
          vals_xs.push(val.start.to_number());
          vals_ys.push(val.from.to_number());
          vals_xs.push(val.end.to_number());
          vals_ys.push(val.to.to_number());
        }
        vals_xs.push(this.horizon);
        if (tl.get_values().length > 0)
          vals_ys.push(tl.get_values()[tl.get_values().length - 1].end.to_number());
        else
          vals_ys.push(tl.initial_amount.to_number());
        this.traces.get(id)!.push({ x: vals_xs, y: vals_ys, name: solver.timelines.Timeline.timeline_name(this.payload, tl), type: 'scatter', opacity: 0.7, mode: 'lines', fill: 'tozeroy', yaxis: this.y_axes.get(id) });
        this.traces.get(id)!.push({ x: [this.origin, this.horizon], y: [tl.capacity, tl.capacity], name: 'Capacity', type: 'scatter', opacity: 0.7, mode: 'lines', yaxis: this.y_axes.get(id) });
        if (i == 1)
          this.layout['yaxis'] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, range: [0, tl.capacity] };
        else
          this.layout[`yaxis${i}`] = { title: solver.timelines.Timeline.timeline_name(this.payload, tl), domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, range: [0, tl.capacity] };
      }

      start_domain += domain_size;
      i++;
    }

    Plotly.react(this.element, Array.from(this.traces.values()).flat(), this.layout, this.config);
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

  override unmounting(): void { this.payload.remove_solver_listener(this); }
}

function values_y(start: number, end: number, ends: number[]): number {
  for (let i = 0; i < ends.length; i++)
    if (ends[i] <= start) {
      ends[i] = end;
      return i;
    }
  ends.push(end);
  return ends.length - 1;
}