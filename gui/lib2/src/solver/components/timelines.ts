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

      const x_axis_g = container.append('g')
        .attr('class', 'x-axis')
        .attr('transform', `translate(0, ${this.height - this.margin.bottom})`)
        .call(this.x_axis);

      // Define zoom behavior
      const zoom = d3.zoom<SVGSVGElement, unknown>()
        .on('zoom', (event: d3.D3ZoomEvent<SVGSVGElement, unknown>) => {
          // Update and redraw the x-axis with the transformed scale
          x_axis_g.call(this.x_axis.scale(event.transform.rescaleX(this.x_scale)));

          // Update the container's transform to reflect the zoom
          this.container!.attr('transform', `translate(${event.transform.x}, 0) scale(${event.transform.k}, 1)`);
        });

      // Attach the zoom behavior to the container
      container.call(zoom);

      this.payload.add_solver_listener(this);
      this.state_changed();
    }

    state_changed(): void {
      const origin = (this.payload.get_exprs().get('origin') as solver.values.Real).val.to_number();
      const horizon = (this.payload.get_exprs().get('horizon') as solver.values.Real).val.to_number();

      this.height = 200 * this.payload.get_timelines().size;
      this.container!.attr("height", this.height).attr("viewBox", `0 0 ${this.width} ${this.height}`);

      this.x_scale.domain([origin, horizon]);

      // Create a group (<g>) for each timeline
      const timelines = this.container!.selectAll('g.timeline')
        .data(this.payload.get_timelines()) // Bind data to the groups
        .enter()
        .append('g')
        .attr('class', 'timeline') // Add class to each <g>
        .attr('id', d => d[1].get_id()) // Use names as IDs, but format them
        .attr('transform', (_, i) => `translate(0, ${i * 50})`)
        .each(function (tl) {
          ChartManager.get_instance().get_chart_generator(tl[1].get_type()).make_chart(tl[1]);
        }); // Position groups (stacked vertically)

      // Add placeholder content to each timeline group for visualization
      timelines.append('rect')
        .attr('x', this.x_scale(origin))
        .attr('y', 10)
        .attr('width', this.x_scale(horizon) - this.x_scale(origin))
        .attr('height', 30)
        .attr('fill', 'steelblue');

      timelines.append('text')
        .attr('x', 10)
        .attr('y', 30)
        .text(d => d[1].get_name())
        .attr('fill', 'black')
        .style('font-size', '12px')
        .style('alignment-baseline', 'middle');

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