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

  state_changed(_state: solver.ExecutionState): void { this.render(); }
  flaw_created(_flaw: solver.graph.Flaw): void { }
  flaw_state_changed(_flaw: solver.graph.Flaw): void { }
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

  private render(): void {
    this.element.innerHTML = to_icon(this.payload.get_state()) + ' ' + this.payload.get_name();
  }

  override unmounting(): void {
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

function to_icon(state: solver.ExecutionState): string[] {
  switch (state) {
    case solver.ExecutionState.reasoning:
    case solver.ExecutionState.adapting: return icon(faBrain).html;
    case solver.ExecutionState.idle: return icon(faPauseCircle).html;
    case solver.ExecutionState.executing: return icon(faPlayCircle).html;
    case solver.ExecutionState.finished: return icon(faCheckCircle).html;
    case solver.ExecutionState.failed: return icon(faXmarkCircle).html;
  }
}