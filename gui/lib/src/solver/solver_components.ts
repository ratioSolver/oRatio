import { App, Component, AnchorComponent, UListComponent } from '../app';
import { solver } from "./solver";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faBrain, faPauseCircle, faPlayCircle, faCheckCircle, faXmarkCircle } from '@fortawesome/free-solid-svg-icons'
import { TimelinesChart } from './solver_timelines';
import { SolverGraph } from './solver_graph';

library.add(faBrain, faPauseCircle, faPlayCircle, faCheckCircle, faXmarkCircle);

export class SolverAnchor extends AnchorComponent<solver.Solver> implements solver.SolverListener {

  constructor(solver: solver.Solver) {
    super(solver);
    solver.add_solver_listener(this);
    this.element.addEventListener('click', () => { App.get_instance().selected_component(this); });
  }

  init(items: Map<string, solver.values.Value>, atoms: Map<number, solver.values.Atom>, state: solver.SolverState, flaws: Map<number, solver.graph.Flaw>, resolvers: Map<number, solver.graph.Resolver>, c_flaw: solver.graph.Flaw | null, c_resolver: solver.graph.Resolver | null): void { this.render(); }
  state_changed(state: solver.SolverState): void { this.render(); }
  flaw_created(flaw: solver.graph.Flaw): void { }
  flaw_cost_changed(flaw: solver.graph.Flaw): void { }
  current_flaw(flaw: solver.graph.Flaw | null): void { }
  resolver_created(resolver: solver.graph.Resolver): void { }
  current_resolver(resolver: solver.graph.Resolver | null): void { }

  private render(): void {
    this.element.innerHTML = to_icon(this.payload.get_state()) + ' ' + this.payload.get_name();
  }

  unmounting(): void {
    this.payload.remove_solver_listener(this);
    if (App.get_instance().get_selected_component() === this)
      App.get_instance().selected_component(null);
  }
}

class SolverListItem extends Component<solver.Solver, HTMLLIElement> {

  private solver_anchor: SolverAnchor;

  constructor(solver: solver.Solver) {
    super(solver, document.createElement('li'));
    this.solver_anchor = new SolverAnchor(solver);
    this.element.appendChild(this.solver_anchor.element);
  }
}

export class SolverListComponent extends UListComponent<solver.Solver> {

  constructor(payload: SolverListItem[]) {
    super(payload, (s0: solver.Solver, s1: solver.Solver) => s0.get_name().localeCompare(s1.get_name()));
  }
}

export class SolverComponent extends Component<solver.Solver, HTMLDivElement> {

  private timelines_chart: TimelinesChart;
  private graph_component: SolverGraph;

  constructor(solver: solver.Solver) {
    super(solver, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    const fragment = document.createDocumentFragment();
    const pills = document.createElement('ul');
    pills.classList.add('nav', 'nav-pills', 'mb-3');

    const timelines_pill = document.createElement('li');
    timelines_pill.role = 'presentation';
    const timelines_pill_link = document.createElement('a');
    timelines_pill_link.classList.add('nav-link', 'active');
    timelines_pill_link.id = 'timelines-tab';
    timelines_pill_link.setAttribute('data-bs-toggle', 'pill');
    timelines_pill_link.setAttribute('data-bs-target', '#slv-' + solver.get_id() + '-timelines');
    timelines_pill_link.setAttribute('role', 'tab');
    timelines_pill_link.setAttribute('aria-controls', 'timelines');
    timelines_pill_link.setAttribute('aria-selected', 'true');
    timelines_pill_link.innerText = 'Timelines';
    timelines_pill.appendChild(timelines_pill_link);
    pills.appendChild(timelines_pill);

    const graph_pill = document.createElement('li');
    graph_pill.role = 'presentation';
    const graph_pill_link = document.createElement('a');
    graph_pill_link.classList.add('nav-link');
    graph_pill_link.id = 'graph-tab';
    graph_pill_link.setAttribute('data-bs-toggle', 'pill');
    graph_pill_link.setAttribute('data-bs-target', '#slv-' + solver.get_id() + '-graph');
    graph_pill_link.setAttribute('role', 'tab');
    graph_pill_link.setAttribute('aria-controls', 'graph');
    graph_pill_link.setAttribute('aria-selected', 'false');
    graph_pill_link.innerText = 'Graph';
    graph_pill.appendChild(graph_pill_link);
    pills.appendChild(graph_pill);

    fragment.appendChild(pills);

    const tab_content = document.createElement('div');
    tab_content.classList.add('tab-content');
    tab_content.id = 'slv-' + solver.get_id() + '-tab-content';

    const timelines = document.createElement('div');
    timelines.classList.add('tab-pane', 'fade', 'show', 'active');
    timelines.id = 'slv-' + solver.get_id() + '-timelines';
    timelines.setAttribute('role', 'tabpanel');
    timelines.setAttribute('aria-labelledby', 'timelines-tab');
    tab_content.appendChild(timelines);

    const graph = document.createElement('div');
    graph.classList.add('tab-pane', 'fade');
    graph.id = 'slv-' + solver.get_id() + '-graph';
    graph.setAttribute('role', 'tabpanel');
    graph.setAttribute('aria-labelledby', 'graph-tab');
    tab_content.appendChild(graph);

    fragment.appendChild(tab_content);

    this.timelines_chart = new TimelinesChart(solver);
    this.graph_component = new SolverGraph(solver);
  }

  unmounting(): void {
    this.timelines_chart.remove();
    this.graph_component.remove();
  }
}

function to_icon(state: solver.SolverState): string[] {
  switch (state) {
    case solver.SolverState.reasoning:
    case solver.SolverState.adapting: return icon(faBrain).html;
    case solver.SolverState.idle: return icon(faPauseCircle).html;
    case solver.SolverState.executing: return icon(faPlayCircle).html;
    case solver.SolverState.finished: return icon(faCheckCircle).html;
    case solver.SolverState.failed: return icon(faXmarkCircle).html;
  }
}