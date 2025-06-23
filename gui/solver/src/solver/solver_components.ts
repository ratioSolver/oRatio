import { Component, SelectorGroup, UListComponent, UListElement } from '@ratiosolver/flick';
import { solver } from "./solver";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faBrain, faPauseCircle, faPlayCircle, faCheckCircle, faXmarkCircle } from '@fortawesome/free-solid-svg-icons'
import { TimelinesChart } from './solver_timelines';
import { SolverGraph } from './solver_graph';

library.add(faBrain, faPauseCircle, faPlayCircle, faCheckCircle, faXmarkCircle);

export class SolverElement extends UListElement<solver.Solver> implements solver.SolverListener {

  constructor(group: SelectorGroup, solver: solver.Solver) {
    super(group, solver, to_icon(solver.get_state()), solver.get_name(), () => new SolverComponent(this.payload));
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
  execution_state_changed(state: solver.ExecutionState): void { this.set_icon(to_icon(state)); }
  tick(_time: solver.values.Rational): void { }
  starting(_atoms: solver.values.Atom[]): void { }
  start(_atoms: solver.values.Atom[]): void { }
  ending(_atoms: solver.values.Atom[]): void { }
  end(_atoms: solver.values.Atom[]): void { }
}

export class SolverList extends UListComponent<solver.Solver> implements solver.SolverSetListener {

  private group: SelectorGroup;

  constructor(group: SelectorGroup = new SelectorGroup(), slvs: solver.Solver[] = []) {
    super(slvs.map(slv => new SolverElement(group, slv)), (t0: solver.Solver, t1: solver.Solver) => t0.get_name() === t1.get_name() ? 0 : (t0.get_name() < t1.get_name() ? -1 : 1));
    this.group = group;
    this.element.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');
    solver.SolverSet.get_instance().add_solver_set_listener(this);
  }

  init(_solvers: Map<number, solver.Solver>): void { }
  solver_created(solver: solver.Solver): void { this.add_child(new SolverElement(this.group, solver)); }
  solver_deleted(id: number): void { this.remove_child(this.children.find(child => child.payload.get_id() === id)!); }

  override unmounting(): void { solver.SolverSet.get_instance().remove_solver_set_listener(this); }
}

export class SolverComponent extends Component<solver.Solver, HTMLDivElement> {

  private selected_comp: Component<any, HTMLElement> | null = null;

  constructor(solver: solver.Solver) {
    super(solver, document.createElement('div'));
    this.element.id = 'slv-' + solver.get_id();
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    const fragment = document.createDocumentFragment();
    const pills = document.createElement('ul');
    pills.classList.add('nav', 'nav-pills', 'mb-3');

    const timelines_pill = document.createElement('li');
    timelines_pill.classList.add('nav-item');
    timelines_pill.role = 'presentation';
    const timelines_button = document.createElement('button');
    timelines_button.classList.add('nav-link', 'active');
    timelines_button.id = 'timelines-tab';
    timelines_button.type = 'button';
    timelines_button.addEventListener('click', () => {
      this.selected_comp?.remove();
      this.selected_comp = new TimelinesChart(solver);
      this.add_child(this.selected_comp);
    });
    timelines_button.setAttribute('data-bs-toggle', 'pill');
    timelines_button.setAttribute('role', 'tab');
    timelines_button.setAttribute('aria-controls', 'timelines');
    timelines_button.setAttribute('aria-selected', 'true');
    timelines_button.innerText = 'Timelines';
    timelines_pill.appendChild(timelines_button);
    pills.appendChild(timelines_pill);

    const graph_pill = document.createElement('li');
    graph_pill.classList.add('nav-item');
    graph_pill.role = 'presentation';
    const graph_button = document.createElement('button');
    graph_button.classList.add('nav-link');
    graph_button.id = 'graph-tab';
    graph_button.type = 'button';
    graph_button.addEventListener('click', () => {
      this.selected_comp?.remove();
      this.selected_comp = new SolverGraph(solver);
      this.add_child(this.selected_comp);
    });
    graph_button.setAttribute('data-bs-toggle', 'pill');
    graph_button.setAttribute('role', 'tab');
    graph_button.setAttribute('aria-controls', 'graph');
    graph_button.setAttribute('aria-selected', 'false');
    graph_button.innerText = 'Graph';
    graph_pill.appendChild(graph_button);
    pills.appendChild(graph_pill);

    fragment.appendChild(pills);

    this.element.appendChild(fragment);
  }

  override unmounting(): void { if (this.selected_comp) this.selected_comp.unmounting(); }
}

function to_icon(state: solver.ExecutionState): Element {
  switch (state) {
    case solver.ExecutionState.reasoning:
    case solver.ExecutionState.adapting: return icon(faBrain).node[0];
    case solver.ExecutionState.idle: return icon(faPauseCircle).node[0];
    case solver.ExecutionState.executing: return icon(faPlayCircle).node[0];
    case solver.ExecutionState.finished: return icon(faCheckCircle).node[0];
    case solver.ExecutionState.failed: return icon(faXmarkCircle).node[0];
  }
}