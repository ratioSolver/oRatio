import { Settings, AppComponent, App, Connection, SolverComponent } from 'ratio-gui';

class oRatio extends AppComponent {

  solver_component: SolverComponent | null = null;

  constructor() {
    super();

    Connection.get_instance().connect();
  }

  populate_navbar(container: HTMLDivElement): void {
    const brand = document.createElement('a');
    brand.classList.add('navbar-brand');
    brand.href = '#';
    brand.textContent = 'oRatio';
    container.appendChild(brand);
  }

  received_message(message: any): void {
    console.log(message);
    this.solver_component = new SolverComponent(message);
  }
}

new oRatio();
App.get_instance().selected_component(null);