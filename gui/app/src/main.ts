import { Settings, AppComponent, App, Connection, BrandComponent, NavbarContent } from '@ratiosolver/flick';
import { solver, TimelinesChart, SolverGraph } from '@ratiosolver/solver';
import './styles.css';

Settings.get_instance().load_settings({ ws_path: '/ratio' });

class ChartSelector extends NavbarContent implements solver.SolverSetListener {

  private solver: solver.Solver | null = null;

  constructor() {
    super();

    const pills = document.createElement('ul');
    pills.classList.add('nav', 'nav-pills', 'ml-2');

    const timelines_pill = document.createElement('li');
    timelines_pill.classList.add('nav-item');
    timelines_pill.role = 'presentation';
    const timelines_button = document.createElement('button');
    timelines_button.classList.add('nav-link', 'active');
    timelines_button.id = 'timelines-tab';
    timelines_button.type = 'button';
    timelines_button.setAttribute('data-bs-toggle', 'pill');
    timelines_button.setAttribute('role', 'tab');
    timelines_button.setAttribute('aria-controls', 'timelines');
    timelines_button.setAttribute('aria-selected', 'true');
    timelines_button.innerText = 'Timelines';
    timelines_button.addEventListener('click', () => { App.get_instance().selected_component(new TimelinesChart(this.solver!)); });
    timelines_pill.appendChild(timelines_button);
    pills.appendChild(timelines_pill);

    const graph_pill = document.createElement('li');
    graph_pill.classList.add('nav-item');
    graph_pill.role = 'presentation';
    const graph_button = document.createElement('button');
    graph_button.classList.add('nav-link');
    graph_button.id = 'graph-tab';
    graph_button.type = 'button';
    graph_button.setAttribute('data-bs-toggle', 'pill');
    graph_button.setAttribute('role', 'tab');
    graph_button.setAttribute('aria-controls', 'graph');
    graph_button.setAttribute('aria-selected', 'false');
    graph_button.innerText = 'Graph';
    graph_button.addEventListener('click', () => { App.get_instance().selected_component(new SolverGraph(this.solver!)); });
    graph_pill.appendChild(graph_button);
    pills.appendChild(graph_pill);

    this.node.appendChild(pills);

    solver.SolverSet.get_instance().add_solver_set_listener(this);
  }

  init(solvers: Map<number, solver.Solver>): void {
    if (solvers.size !== 1)
      throw new Error('Expected exactly one solver');
    this.solver = solvers.values().next().value!;
    App.get_instance().selected_component(new TimelinesChart(this.solver));
  }
  solver_created(_solver: solver.Solver): void { }
  solver_deleted(_id: number): void { }
}

class oRatio extends AppComponent {

  constructor() {
    super(new BrandComponent('oRatio'), new ChartSelector());

    Connection.get_instance().connect();
  }

  override received_message(message: any): void { solver.SolverSet.get_instance().update_solvers(message); }
}

new oRatio();