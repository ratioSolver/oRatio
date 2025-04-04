import * as d3 from 'd3';
import { solver } from '../solver';
import { Component } from 'ratio-core';

export namespace graph {

  const node_width = 80;  // Replace with actual node width
  const node_height = 30; // Replace with actual node height

  interface GraphNode extends d3.SimulationNodeDatum {
    payload: solver.graph.Flaw | solver.graph.Resolver;
    entering: GraphLink[];
    exiting: GraphLink[];
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
      this.container.append('defs').selectAll('marker').data(['end']).enter().append('marker').attr('id', d => d).attr('viewBox', '0 -5 10 10').attr('refX', 0).attr('refY', 0).attr('orient', 'auto').attr('markerWidth', 6).attr('markerHeight', 6).attr('xoverflow', 'visible').append('path').attr('d', 'M0,-5L10,0L0,5').attr('fill', 'black');

      this.simulation = d3.forceSimulation(Array.from(this.nodes.values()))
        .force('link', d3.forceLink<GraphNode, GraphLink>(this.links).id(d => d.payload.get_id()).distance(100))
        .force('charge', d3.forceManyBody().strength(-400))
        .force('center', d3.forceCenter(this.width / 2, this.height / 2));

      for (const [id, f] of this.payload.get_flaws())
        this.nodes.set(id, { payload: f, entering: [], exiting: [] });

      for (const [id, r] of this.payload.get_resolvers())
        this.nodes.set(id, { payload: r, entering: [], exiting: [] });

      for (const [_, gn] of this.nodes)
        if (gn.payload instanceof solver.graph.Flaw) {
          for (const c of gn.payload.get_supports()) {
            const fr = { source: gn, target: this.nodes.get(c.get_id())! }; // the flaw-resolver links..
            gn.exiting.push(fr);
            this.links.push(fr);
          }
        } else if (gn.payload instanceof solver.graph.Resolver) {
          const fn = this.nodes.get(gn.payload.get_flaw().get_id())!;
          const rf = { source: gn, target: fn }; // the resolver-flaw links..
          gn.exiting.push(rf);
          fn.entering.push(rf);
          this.links.push(rf);
        }

      this.payload.add_solver_listener(this);
      this.state_changed();
    }

    state_changed(): void { }
    flaw_created(f: solver.graph.Flaw): void {
      const fn: GraphNode = { payload: f, entering: [], exiting: [] };
      for (const c of f.get_causes()) {
        const fr = { source: fn, target: this.nodes.get(c.get_id())! }; // the flaw-resolver links..
        fn.exiting.push(fr);
        this.links.push(fr);
      }
      this.nodes.set(f.get_id(), fn);

      this.update_graph();
    }
    flaw_state_changed(_flaw: solver.graph.Flaw): void { this.update_graph(); }
    flaw_position_changed(_flaw: solver.graph.Flaw): void { this.update_graph(); }
    flaw_cost_changed(_flaw: solver.graph.Flaw): void { this.update_graph(); }
    current_flaw(_flaw: solver.graph.Flaw | null): void { this.update_graph(); }
    resolver_created(r: solver.graph.Resolver): void {
      const rn: GraphNode = { payload: r, entering: [], exiting: [] };
      const fn = this.nodes.get(r.get_flaw().get_id())!;
      const rf = { source: rn, target: fn }; // the resolver-flaw links..
      rn.exiting.push(rf);
      fn.entering.push(rf);
      this.links.push(rf);

      this.nodes.set(r.get_id(), rn);

      this.update_graph();
    }
    resolver_state_changed(_resolver: solver.graph.Resolver): void { this.update_graph(); }
    current_resolver(_resolver: solver.graph.Resolver | null): void { this.update_graph(); }
    causal_link_added(_flaw: solver.graph.Flaw, _resolver: solver.graph.Resolver): void { this.update_graph(); }

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
        .data(nodes, d => d.payload.get_id()); // Use a key function to track nodes by `id`

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
        .attr('width', node_width)
        .attr('height', node_height)
        .attr('x', -node_width / 2)
        .attr('y', -node_height / 2)
        .attr('rx', 5)
        .attr('ry', 5)
        .attr('fill', '#69b3a2');

      // Append label to new nodes
      node_enter.append('text')
        .text(d => d.payload.to_string())
        .attr('text-anchor', 'middle')
        .attr('alignment-baseline', 'middle')
        .attr('fill', '#000');

      // Restart the simulation with new data
      this.simulation!
        .nodes(nodes) // Update node data in the simulation
        .on('tick', () => {
          // Update link positions
          this.container!.selectAll<SVGLineElement, GraphLink>('.link')
            .attr('x1', d => {
              let src = intersect({ x: d.source.x! - node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x! - node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! - node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x! + node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! + node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x! + node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! + node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x! - node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              return src ? src.x : d.source.x!;
            })
            .attr('y1', d => {
              let src = intersect({ x: d.source.x! - node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x! - node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! - node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x! + node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! + node_width / 2, y: d.source.y! + node_height / 2 }, { x: d.source.x! + node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!src) src = intersect({ x: d.source.x! + node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x! - node_width / 2, y: d.source.y! - node_height / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              return src ? src.y : d.source.y!;
            })
            .attr('x2', d => {
              let trgt = intersect({ x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              return trgt ? trgt.x : d.target.x!;
            })
            .attr('y2', d => {
              let trgt = intersect({ x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! + node_height * 1.5 / 2 }, { x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              if (!trgt) trgt = intersect({ x: d.target.x! + node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.target.x! - node_width * 1.2 / 2, y: d.target.y! - node_height * 1.5 / 2 }, { x: d.source.x!, y: d.source.y! }, { x: d.target.x!, y: d.target.y! });
              return trgt ? trgt.y : d.target.y!;
            });

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

  type Point = { x: number; y: number };

  function intersect(p0: Point, p1: Point, p2: Point, p3: Point): Point | undefined {
    const s1_x = p1.x - p0.x, s1_y = p1.y - p0.y, s2_x = p3.x - p2.x, s2_y = p3.y - p2.y;

    const s = (-s1_y * (p0.x - p2.x) + s1_x * (p0.y - p2.y)) / (-s2_x * s1_y + s1_x * s2_y);
    const t = (s2_x * (p0.y - p2.y) - s2_y * (p0.x - p2.x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
      return { x: p0.x + (t * s1_x), y: p0.y + (t * s1_y) };
    else
      return undefined;
  }
}