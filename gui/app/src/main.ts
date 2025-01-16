import { Settings, AppComponent, App, Connection, SolverComponent, solver } from 'ratio-lib';
import './styles.css';

Settings.get_instance().load_settings({ port: 8080, ws_path: 'ratio' });

class oRatio extends AppComponent implements solver.SolverSetListener {

  solver_component: SolverComponent | null = null;

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
  }

  init(solvers: Map<number, solver.Solver>): void {
    if (solvers.size !== 1)
      throw new Error('Expected exactly one solver');
    this.solver_component = new SolverComponent(solvers.values()!.next()!.value!);
    App.get_instance().selected_component(this.solver_component);
  }
  solver_created(_: solver.Solver): void { }
  solver_deleted(_: number): void { }

  received_message(message: any): void { solver.SolverSet.get_instance().update_solvers(message); }
}

new oRatio();
App.get_instance().selected_component(null);