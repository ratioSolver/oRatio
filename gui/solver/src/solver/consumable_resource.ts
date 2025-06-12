import { PlotData } from "plotly.js-dist-min";
import { IntervalMessage, RationalMessage, solver, TimelineMsg } from "./solver";

export namespace consumable_resource {

  export class ConsumableResourceTimelineGenerator extends solver.timeline.TimelineGenerator<ConsumableResourceTimelineValue> {

    constructor() { super('ConsumableResource'); }

    override make_timeline(slv: solver.Solver, tml: ConsumableResourceTimelineMessage): ConsumableResourceTimeline {
      const cr_vals = tml.values.map(v => {
        const atms = v.atoms.map(atm => slv._atoms.get(atm)!);
        return { atoms: atms, from: solver.values.Rational.make_rational(v.from), to: solver.values.Rational.make_rational(v.to), start: solver.values.Rational.make_rational(v.start), end: solver.values.Rational.make_rational(v.end) };
      });
      return new ConsumableResourceTimeline(slv, tml.id, tml.name, solver.values.Rational.make_rational(tml.capacity), solver.values.Rational.make_rational(tml.initial_amount), cr_vals);
    }
  }

  type ConsumableResourceTimelineValue = { atoms: solver.values.Atom[], from: solver.values.Rational, to: solver.values.Rational } & solver.timeline.Interval;

  class ConsumableResourceTimeline extends solver.timeline.Timeline<ConsumableResourceTimelineValue> {

    capacity: solver.values.Rational;
    initial_amount: solver.values.Rational;

    constructor(slv: solver.Solver, id: number, name: string, capacity: solver.values.Rational, initial_amount: solver.values.Rational, values: ConsumableResourceTimelineValue[]) {
      super(slv, 'ConsumableResource', id, name, values);
      this.capacity = capacity;
      this.initial_amount = initial_amount;
    }

    override to_string(value: ConsumableResourceTimelineValue, expressive = false): string {
      if (expressive)
        switch (value.atoms.length) {
          case 0:
            return `- (${value.start.to_string()} - ${value.end.to_string()})`;
          case 1:
            return `${value.start.to_string()} - ${value.end.to_string()} ${value.atoms[0].to_string(this.slv, expressive)}`;
          default:
            return `${value.start.to_string()} - ${value.end.to_string()} {${value.atoms.map(atom => atom.to_string(this.slv, expressive)).join(', ')}} (${value.start.to_string()} - ${value.end.to_string()})`;
        }
      else
        switch (value.atoms.length) {
          case 0:
            return '-';
          default:
            return `${value.start.to_string()} - ${value.end.to_string()}`;
        }
    }
  }

  export class ConsumableResourceChartGenerator extends solver.chart.ChartGenerator {

    constructor() { super('ConsumableResource'); }

    make_chart(timeline: ConsumableResourceTimeline): ConsumableResourceChart { return new ConsumableResourceChart(timeline); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class ConsumableResourceChart implements solver.chart.Chart {

    private readonly data: Partial<PlotData>[] = [];
    private readonly timeline: ConsumableResourceTimeline;

    constructor(timeline: ConsumableResourceTimeline) {
      this.timeline = timeline;
      this.set_data(timeline);
    }

    set_data(timeline: ConsumableResourceTimeline): Partial<PlotData>[] {
      this.data.length = 0;
      const origin = (timeline.get_solver().get_exprs().get('origin') as solver.values.Real).val.to_number();
      const horizon = (timeline.get_solver().get_exprs().get('horizon') as solver.values.Real).val.to_number();
      const xs = [origin];
      const ys = [timeline.initial_amount.to_number()];
      for (const val of timeline.get_values()) {
        xs.push(val.start.to_number());
        ys.push(val.from.to_number());
        xs.push(val.end.to_number());
        ys.push(val.to.to_number());
      }
      this.data.push({ x: xs, y: ys, name: timeline.get_name(), type: 'scatter', opacity: 0.7, mode: 'lines', fill: 'tozeroy' });
      this.data.push({ x: [origin, horizon], y: [timeline.capacity.to_number(), timeline.capacity.to_number()], name: 'Capacity', type: 'scatter', opacity: 0.7, mode: 'lines' });
      return this.data;
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): number[] { return [0, this.timeline.capacity.to_number()]; }
  }
}

interface ConsumableResourceTimelineMessage extends TimelineMsg<{ atoms: number[]; from: RationalMessage; to: RationalMessage } & IntervalMessage> {

  capacity: RationalMessage;
  initial_amount: RationalMessage;
}
