import { Settings, AppComponent, App, Connection, solver, TimelinesChart, SolverGraph } from 'ratio-lib';
import './styles.css';

Settings.get_instance().load_settings({ port: 8080, ws_path: 'ratio' });

class oRatio extends AppComponent implements solver.SolverSetListener {

  private solver: solver.Solver | null = null;

  constructor() {
    super();

    solver.SolverSet.get_instance().add_solver_set_listener(this);
    Connection.get_instance().connect();
  }

  populate_navbar(container: HTMLDivElement): void {
    const brand = document.createElement('a');
    brand.classList.add('navbar-brand');
    brand.href = '#';
    brand.textContent = 'oRatio';
    container.appendChild(brand);

    const pills = document.createElement('ul');
    pills.classList.add('nav', 'nav-pills', 'ml-2');

    const timelines_pill = document.createElement('li');
    timelines_pill.classList.add('nav-item');
    timelines_pill.role = 'presentation';
    const timelines_pill_link = document.createElement('button');
    timelines_pill_link.classList.add('nav-link', 'active');
    timelines_pill_link.id = 'timelines-tab';
    timelines_pill_link.setAttribute('data-bs-toggle', 'pill');
    timelines_pill_link.setAttribute('role', 'tab');
    timelines_pill_link.setAttribute('aria-controls', 'timelines');
    timelines_pill_link.setAttribute('aria-selected', 'true');
    timelines_pill_link.innerText = 'Timelines';
    timelines_pill_link.addEventListener('click', () => { App.get_instance().selected_component(new TimelinesChart(this.solver!)); });
    timelines_pill.appendChild(timelines_pill_link);
    pills.appendChild(timelines_pill);

    const graph_pill = document.createElement('li');
    graph_pill.classList.add('nav-item');
    graph_pill.role = 'presentation';
    const graph_pill_link = document.createElement('button');
    graph_pill_link.classList.add('nav-link');
    graph_pill_link.id = 'graph-tab';
    graph_pill_link.setAttribute('data-bs-toggle', 'pill');
    graph_pill_link.setAttribute('role', 'tab');
    graph_pill_link.setAttribute('aria-controls', 'graph');
    graph_pill_link.setAttribute('aria-selected', 'false');
    graph_pill_link.innerText = 'Graph';
    graph_pill_link.addEventListener('click', () => { App.get_instance().selected_component(new SolverGraph(this.solver!)); });
    graph_pill.appendChild(graph_pill_link);
    pills.appendChild(graph_pill);

    container.appendChild(pills);
  }

  init(solvers: Map<number, solver.Solver>): void {
    if (solvers.size !== 1)
      throw new Error('Expected exactly one solver');
    this.solver = solvers.values().next().value!;
    App.get_instance().selected_component(new TimelinesChart(this.solver));
  }
  solver_created(_: solver.Solver): void { }
  solver_deleted(_: number): void { }

  received_message(message: any): void { solver.SolverSet.get_instance().update_solvers(message); }
}

new oRatio();