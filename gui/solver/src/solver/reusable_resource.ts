import { PlotData } from "plotly.js-dist-min";
import { IntervalMessage, RationalMessage, solver, TimelineMsg } from "./solver";

export namespace reusable_resource {

  export class ReusableResourceTimelineGenerator extends solver.timeline.TimelineGenerator<ReusableResourceTimelineValue> {

    constructor() { super('ReusableResource'); }

    override make_timeline(slv: solver.Solver, tml: ReusableResourceTimelineMessage): ReusableResourceTimeline {
      const rr_vals = tml.values.map(v => {
        const atms = v.atoms.map(atm => slv._atoms.get(atm)!);
        return { atoms: atms, amount: solver.values.Rational.make_rational(v.amount), start: solver.values.Rational.make_rational(v.start), end: solver.values.Rational.make_rational(v.end) };
      });
      return new ReusableResourceTimeline(slv, tml.id, tml.name, solver.values.Rational.make_rational(tml.capacity), rr_vals);
    }
  }

  type ReusableResourceTimelineValue = { atoms: solver.values.Atom[], amount: solver.values.Rational } & solver.timeline.Interval;

  class ReusableResourceTimeline extends solver.timeline.Timeline<ReusableResourceTimelineValue> {

    capacity: solver.values.Rational;

    constructor(slv: solver.Solver, id: number, name: string, capacity: solver.values.Rational, values: ReusableResourceTimelineValue[]) {
      super(slv, 'ReusableResource', id, name, values);
      this.capacity = capacity;
    }

    override to_string(value: ReusableResourceTimelineValue, expressive = false): string {
      if (expressive)
        switch (value.atoms.length) {
          case 0:
            return `0 (${value.start.to_string()} - ${value.end.to_string()})`;
          case 1:
            return value.amount.to_string() + ' ' + value.atoms[0].to_string(this.slv, expressive);
          default:
            return value.amount.to_string() + ` {${value.atoms.map(atom => atom.to_string(this.slv, expressive)).join(', ')}} (${value.start.to_string()} - ${value.end.to_string()})`;
        }
      else
        return value.amount.to_string();
    }
  }

  export class ReusableResourceChartGenerator extends solver.chart.ChartGenerator {

    constructor() { super('ReusableResource'); }

    make_chart(timeline: ReusableResourceTimeline): ReusableResourceChart { return new ReusableResourceChart(timeline); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class ReusableResourceChart implements solver.chart.Chart {

    private readonly data: Partial<PlotData>[] = [];
    private readonly timeline: ReusableResourceTimeline;

    constructor(timeline: ReusableResourceTimeline) {
      this.timeline = timeline;
      this.set_data(timeline);
    }

    set_data(timeline: ReusableResourceTimeline): Partial<PlotData>[] {
      this.data.length = 0;
      const origin = (timeline.get_solver().get_exprs().get('origin') as solver.values.Real).val.to_number();
      const horizon = (timeline.get_solver().get_exprs().get('horizon') as solver.values.Real).val.to_number();
      const xs = [origin];
      const ys = [0];
      for (const val of timeline.get_values()) {
        xs.push(val.start.to_number());
        ys.push(val.amount.to_number());
        xs.push(val.end.to_number());
        ys.push(val.amount.to_number());
      }
      this.data.push({ x: xs, y: ys, name: timeline.get_name(), type: 'scatter', opacity: 0.7, mode: 'lines', fill: 'tozeroy' });
      this.data.push({ x: [origin, horizon], y: [timeline.capacity.to_number(), timeline.capacity.to_number()], name: 'Capacity', type: 'scatter', opacity: 0.7, mode: 'lines' });
      return this.data;
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): number[] { return [0, this.timeline.capacity.to_number()]; }
  }
}

interface ReusableResourceTimelineMessage extends TimelineMsg<{ atoms: number[]; amount: RationalMessage } & IntervalMessage> {

  capacity: RationalMessage;
}
