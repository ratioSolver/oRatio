import * as d3 from 'd3';
import { solver } from '../solver';
import { Component } from 'ratio-core';

export namespace graph {

  type GraphNode = d3.SimulationNodeDatum & { id: string };

  export class SolverGraph extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

    private readonly width = 800;
    private readonly height = 600;
    private readonly nodes: GraphNode[] = [];
    private readonly edges: d3.SimulationLinkDatum<GraphNode>[] = [];

    private container: d3.Selection<SVGSVGElement, unknown, HTMLElement, any> | undefined;
    private simulation: d3.Simulation<GraphNode, d3.SimulationLinkDatum<GraphNode>> | undefined;

    constructor(solver: solver.Solver) {
      super(solver, document.createElement('div'));
      this.element.id = 'slv-' + solver.get_id() + '-graph';
      this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    }

    override mounted(): void {
      this.container = d3.select('#slv-' + this.payload.get_id() + '-graph').append('svg').attr('viewBox', `0 0 ${this.width} ${this.height}`).attr('width', '100%').attr('height', '100%');
      this.simulation = d3.forceSimulation(this.nodes).force('link', d3.forceLink<GraphNode, d3.SimulationLinkDatum<GraphNode>>(this.edges).id(d => d.id).distance(100)).force('charge', d3.forceManyBody().strength(-400)).force('center', d3.forceCenter(this.width / 2, this.height / 2));

      // Draw links (directed edges)
      const link = this.container.selectAll('.link').data(this.edges).enter().append('line').attr('class', 'link').attr('stroke', 'black').attr('stroke-width', 1.5);

      // Draw arrows for directed edges
      this.container.append('defs').selectAll('marker').data(['end']).enter().append('marker').attr('id', d => d).attr('viewBox', '0 -5 10 10').attr('refX', 15).attr('refY', 0).attr('orient', 'auto').attr('markerWidth', 6).attr('markerHeight', 6).attr('xoverflow', 'visible').append('path').attr('d', 'M0,-5L10,0L0,5').attr('fill', 'black');

      link.attr('marker-end', 'url(#end)'); // Add arrowheads to links

      // Draw nodes
      const node = this.container.selectAll('.node').data(this.nodes).enter().append('circle').attr('class', 'node').attr('r', 10).attr('fill', 'steelblue').call(d3.drag<SVGCircleElement, GraphNode>().on('start', this.dragstarted).on('drag', this.dragged).on('end', this.dragended));

      // Add labels to nodes
      const labels = this.container.selectAll('.label').data(this.nodes).enter().append('text').attr('class', 'label').attr('text-anchor', 'middle').attr('dy', -15).text(d => d.id);

      // Update positions on each tick
      this.simulation.on('tick', () => {
        // Update link positions
        link.attr('x1', d => (d.source as GraphNode).x!).attr('y1', d => (d.source as GraphNode).y!).attr('x2', d => (d.target as GraphNode).x!).attr('y2', d => (d.target as GraphNode).y!);

        // Update node positions
        node.attr('cx', d => d.x!).attr('cy', d => d.y!);

        // Update label positions
        labels.attr('x', d => d.x!).attr('y', d => d.y!);
      });

      this.payload.add_solver_listener(this);
      this.state_changed();
    }

    state_changed(): void { }
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

    // Drag event handlers
    private dragstarted(event: any, d: any) {
      if (!event.active) this.simulation!.alphaTarget(0.3).restart(); // Restart simulation
      d.fx = d.x; // Fix x-coordinate
      d.fy = d.y; // Fix y-coordinate
    }

    private dragged(event: any, d: any) {
      d.fx = event.x; // Update x-coordinate while dragging
      d.fy = event.y; // Update y-coordinate while dragging
    }

    private dragended(event: any, d: any) {
      if (!event.active) this.simulation!.alphaTarget(0); // Stop simulation
      d.fx = null; // Release fixed position
      d.fy = null; // Release fixed position
    }
  }
}