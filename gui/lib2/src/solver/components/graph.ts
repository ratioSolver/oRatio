import * as d3 from 'd3';
import { solver } from '../solver';
import { Component } from 'ratio-core';

export namespace graph {

  interface GraphNode extends d3.SimulationNodeDatum {
    id: number;
    label: string;
  }

  interface GraphLink extends d3.SimulationLinkDatum<GraphNode> {
    source: GraphNode;
    target: GraphNode;
  }

  export class SolverGraph extends Component<solver.Solver, HTMLDivElement> implements solver.SolverListener {

    private readonly width = 800;
    private readonly height = 600;
    private readonly nodes: Map<number, GraphNode> = new Map();
    private readonly links: GraphLink[] = [];

    private container: d3.Selection<SVGSVGElement, unknown, HTMLElement, any> | undefined;
    private simulation: d3.Simulation<GraphNode, GraphLink> | undefined;

    constructor(solver: solver.Solver) {
      super(solver, document.createElement('div'));
      this.element.id = 'slv-' + solver.get_id() + '-graph';
      this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    }

    override mounted(): void {
      this.container = d3.select('#slv-' + this.payload.get_id() + '-graph').append('svg').attr('viewBox', `0 0 ${this.width} ${this.height}`).attr('width', '100%').attr('height', '100%');

      // Draw arrows for directed edges
      this.container.append('defs').selectAll('marker').data(['end']).enter().append('marker').attr('id', d => d).attr('viewBox', '0 -5 10 10').attr('refX', 15).attr('refY', 0).attr('orient', 'auto').attr('markerWidth', 6).attr('markerHeight', 6).attr('xoverflow', 'visible').append('path').attr('d', 'M0,-5L10,0L0,5').attr('fill', 'black');

      this.simulation = d3.forceSimulation(Array.from(this.nodes.values()))
        .force('link', d3.forceLink<GraphNode, GraphLink>(this.links).id(d => d.id).distance(100))
        .force('charge', d3.forceManyBody().strength(-400))
        .force('center', d3.forceCenter(this.width / 2, this.height / 2));

      this.payload.add_solver_listener(this);
      this.state_changed();
    }

    state_changed(): void { }
    flaw_created(_flaw: solver.graph.Flaw): void {
      this.update_graph();
    }
    flaw_state_changed(_flaw: solver.graph.Flaw): void { }
    flaw_position_changed(_flaw: solver.graph.Flaw): void { }
    flaw_cost_changed(_flaw: solver.graph.Flaw): void { }
    current_flaw(_flaw: solver.graph.Flaw | null): void { }
    resolver_created(_resolver: solver.graph.Resolver): void { }
    resolver_state_changed(_resolver: solver.graph.Resolver): void { }
    current_resolver(_resolver: solver.graph.Resolver | null): void { }
    causal_link_added(_flaw: solver.graph.Flaw, _resolver: solver.graph.Resolver): void { }

    private update_graph(): void {
      // Update links (directed edges)
      const link = this.container!.selectAll<SVGLineElement, GraphLink>('.link')
        .data(this.links)
        .enter()
        .append('line')
        .attr('class', 'link')
        .attr('stroke', 'black')
        .attr('stroke-width', 1.5);

      link.attr('marker-end', 'url(#end)'); // Add arrowheads to links

      // Update nodes
      const nodes = Array.from(this.nodes.values());

      const node = this.container!
        .selectAll<SVGGElement, GraphNode>('.node') // Select all existing nodes
        .data(nodes, d => d.id); // Use a key function to track nodes by `id`

      // Create new node groups as needed
      const node_enter = node.enter()
        .append('g')
        .attr('class', 'node')
        .call(
          d3.drag<SVGGElement, GraphNode>()
            .on('start', (event, d) => {
              if (!event.active) this.simulation!.alphaTarget(0.3).restart();
              d.fx = d.x;
              d.fy = d.y;
            })
            .on('drag', (event, d) => {
              d.fx = event.x;
              d.fy = event.y;
            })
            .on('end', (event, d) => {
              if (!event.active) this.simulation!.alphaTarget(0);
              d.fx = null;
              d.fy = null;
            })
        );

      // Append rectangle to new nodes
      node_enter.append('rect')
        .attr('width', 100)
        .attr('height', 30)
        .attr('x', -50)
        .attr('y', -15)
        .attr('rx', 5)
        .attr('ry', 5)
        .attr('fill', '#69b3a2');

      // Append label to new nodes
      node_enter.append('text')
        .text(d => d.label)
        .attr('text-anchor', 'middle')
        .attr('alignment-baseline', 'middle')
        .attr('fill', '#000');

      // Restart the simulation with new data
      this.simulation!
        .nodes(nodes) // Update node data in the simulation
        .on('tick', () => {
          // Update link positions
          this.container!.selectAll<SVGLineElement, GraphLink>('.link')
            .attr('x1', d => d.source.x!)
            .attr('y1', d => d.source.y!)
            .attr('x2', d => d.target.x!)
            .attr('y2', d => d.target.y!);

          // Update node positions
          this.container!.selectAll<SVGGElement, GraphNode>('.node')
            .attr('transform', d => `translate(${d.x},${d.y})`);
        });

      this.simulation!.force<d3.ForceLink<GraphNode, GraphLink>>('link')!.links(this.links); // Update link data in the simulation
      this.simulation!.alpha(1).restart(); // Restart the simulation with the new layout
    }

    execution_state_changed(_state: solver.ExecutionState): void { }
    tick(_time: solver.values.Rational): void { }
    starting(_atoms: solver.values.Atom[]): void { }
    start(_atoms: solver.values.Atom[]): void { }
    ending(_atoms: solver.values.Atom[]): void { }
    end(_atoms: solver.values.Atom[]): void { }
  }
}