import * as d3 from 'd3';
import { solver } from '../solver';
import { Component } from 'ratio-core';

export namespace timeline {

  export class TimelinesChart extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

    private readonly width = 800;
    private height = 600;
    private readonly margin = { top: 20, right: 30, bottom: 40, left: 50 };

    private container: d3.Selection<SVGGElement, unknown, HTMLElement, any> | undefined;
    private readonly x_scale = d3.scaleTime().domain([0, 1]).range([this.margin.left, this.width - this.margin.right]);
    private readonly x_axis = d3.axisBottom(this.x_scale);

    constructor(solver: solver.Solver) {
      super(solver, document.createElement('div'));
      this.element.id = 'slv-' + solver.get_id() + '-timelines';
      this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    }

    override mounted(): void {
      const container = d3.select('#slv-' + this.payload.get_id() + '-timelines')
        .append('svg')
        .attr('viewBox', `0 0 ${this.width} ${this.height}`)
        .attr('width', '100%')
        .attr('height', this.height);

      this.container = container.append('g')
        .attr('class', 'zoom-group');

      // Define zoom behavior
      const zoom = d3.zoom<SVGSVGElement, unknown>()
        .on('zoom', (event) => {
          this.container!.attr('transform', `translate(${event.transform.x}, 0) scale(${event.transform.k}, 1)`);
        });

      // Attach the zoom behavior to the container
      container.call(zoom);

      this.container.append('g')
        .attr('class', 'x-axis')
        .attr('transform', `translate(0, ${this.height - this.margin.bottom})`)
        .call(this.x_axis);

      this.payload.add_solver_listener(this);
      this.state_changed();
    }

    state_changed(): void {
      const origin = (this.payload.get_exprs().get('origin') as solver.values.Real).val.to_number();
      const horizon = (this.payload.get_exprs().get('horizon') as solver.values.Real).val.to_number();

      this.height = 200 * this.payload.get_timelines().size;
      this.container!.attr("height", this.height).attr("viewBox", `0 0 ${this.width} ${this.height}`);

      this.x_scale.domain([origin, horizon]);
      this.container!.select<SVGGElement>('.x-axis').call(this.x_axis);
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
  }

  export class ChartManager {

    private static instance: ChartManager;
    private charts: Map<string, ChartGenerator> = new Map();

    private constructor() {
    }

    public static get_instance(): ChartManager {
      if (!ChartManager.instance)
        ChartManager.instance = new ChartManager();
      return ChartManager.instance;
    }

    add_chart_generator(chart: ChartGenerator) { this.charts.set(chart.get_name(), chart); }
    get_chart_generator(name: string): ChartGenerator { return this.charts.get(name)!; }
  }

  export abstract class ChartGenerator {

    private name: string;

    constructor(name: string) {
      this.name = name;
    }

    get_name(): string { return this.name; }

    show_tick_labels(): boolean | undefined { return undefined; }
    show_grid(): boolean | undefined { return undefined; }

    abstract make_chart(timeline: solver.timeline.Timeline<solver.timeline.TimelineValue>): Timeline;
  }

  export interface Timeline {

    set_data(timeline: solver.timeline.Timeline<solver.timeline.TimelineValue>): void;
    get_data(): void;

    get_range(): number[] | undefined;
  }
}