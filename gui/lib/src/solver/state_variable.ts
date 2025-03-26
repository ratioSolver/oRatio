import { PlotData } from "plotly.js-dist-min";
import { IntervalMessage, solver, TimelineMsg } from "./solver";
import { scaleOrdinal } from "d3-scale";

export namespace state_variable {

  export class StateVariableTimelineGenerator extends solver.timeline.TimelineGenerator<StateVariableTimelineValue> {

    constructor() { super('StateVariable'); }

    override make_timeline(slv: solver.Solver, tml: StateVariableTimelineMessage): StateVariableTimeline {
      const sv_vals = tml.values.map(v => {
        const atms = v.atoms.map(atm => slv._atoms.get(atm)!);
        return { atoms: atms, start: solver.values.Rational.make_rational(v.start), end: solver.values.Rational.make_rational(v.end) };
      });
      return new StateVariableTimeline(slv, tml.id, tml.name, sv_vals);
    }
  }

  type StateVariableTimelineValue = { atoms: solver.values.Atom[] } & solver.timeline.Interval;

  class StateVariableTimeline extends solver.timeline.Timeline<StateVariableTimelineValue> {

    constructor(slv: solver.Solver, id: number, name: string, values: StateVariableTimelineValue[]) {
      super(slv, 'StateVariable', id, name, values);
    }

    override to_string(value: StateVariableTimelineValue, expressive = false): string {
      if (expressive)
        switch (value.atoms.length) {
          case 0:
            return `[] (${value.start.to_string()} - ${value.end.to_string()})`;
          case 1:
            return value.atoms[0].to_string(this.slv, expressive);
          default:
            return `[${value.atoms.map(atom => atom.to_string(this.slv, expressive)).join(', ')}] (${value.start.to_string()} - ${value.end.to_string()})`;
        }
      else
        switch (value.atoms.length) {
          case 0:
            return '[]';
          case 1:
            return value.atoms[0].to_string(this.slv, expressive);
          default:
            return `[${value.atoms.map(atom => atom.to_string(this.slv, expressive)).join(', ')}]`;
        }
    }
  }

  export class StateVariableChartGenerator extends solver.chart.ChartGenerator {

    constructor() { super('StateVariable'); }

    make_chart(timeline: StateVariableTimeline): StateVariableChart { return new StateVariableChart(timeline); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class StateVariableChart implements solver.chart.Chart {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal<number, string>().domain([0, 1, 2]).range(["#9E9E9E", "#4CAF50", "#F44336"]);

    constructor(timeline: StateVariableTimeline) {
      this.set_data(timeline);
    }

    set_data(timeline: StateVariableTimeline): Partial<PlotData>[] {
      this.data.length = 0;
      for (const val of timeline.get_values()) {
        const text = timeline.to_string(val);
        let color: string;
        switch (val.atoms.length) {
          case 0: color = this.colors(0); break;
          case 1: color = this.colors(1); break;
          default: color = this.colors(2); break;
        }
        this.data.push({ x: [val.start.to_number(), val.end.to_number()], y: [1, 1], name: text, text: [text], type: 'scatter', opacity: 0.7, mode: 'text+lines', line: { width: 30, color: color }, textposition: 'middle right' });
      }
      return this.data;
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }
  }
}

interface StateVariableTimelineMessage extends TimelineMsg<{ atoms: number[] } & IntervalMessage> { }
