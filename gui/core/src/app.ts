import { Connection, ConnectionListener } from "./utils/connection";

export class App implements AppListener {

  private static instance: App;
  private selected_comp: Component<any, HTMLElement> | null = null;
  private app_listeners: Set<AppListener> = new Set();

  private constructor() { }

  static get_instance() {
    if (!App.instance)
      App.instance = new App();
    return App.instance;
  }

  toast(info: string): void { for (const listener of this.app_listeners) { listener.toast(info); } }

  get_selected_component(): Component<any, HTMLElement> | null { return this.selected_comp; }

  selected_component(component: Component<any, HTMLElement> | null): void {
    if (this.selected_comp)
      this.selected_comp.remove();
    this.selected_comp = component;
    for (const listener of this.app_listeners) { listener.selected_component(component); }
  }

  add_app_listener(listener: AppListener): void {
    this.app_listeners.add(listener);
  }

  remove_app_listener(listener: AppListener): void {
    this.app_listeners.delete(listener);
  }
}

export interface AppListener {

  toast(info: string): void;

  selected_component(component: Component<any, HTMLElement> | null): void;
}

export abstract class Component<P, E extends HTMLElement> {

  payload: P; // The payload of the component
  element: E; // The element of the component
  protected child_nodes: Set<Component<any, HTMLElement>> = new Set(); // The children of the component

  constructor(payload: P, element: E) {
    this.payload = payload;
    this.element = element;
  }

  add_child(child: Component<any, HTMLElement>): void {
    this.child_nodes.add(child);
    this.element.appendChild(child.element);
    child.mounted();
  }

  remove_child(child: Component<any, HTMLElement>): void {
    if (this.child_nodes.has(child)) {
      this.child_nodes.delete(child);
      child.remove();
    } else
      throw new Error('Child not found');
  }

  remove(): void {
    for (const child of this.child_nodes)
      child.unmounting();
    this.unmounting();
    this.element.remove();
  }

  mounted(): void { }
  unmounting(): void { }
}

export abstract class ListComponent<P, E extends HTMLElement, L extends HTMLElement> extends Component<Component<P, E>[], L> {

  compareFn: (a: P, b: P) => number; // The comparison function
  children: Component<P, E>[] = []; // The children of the list component sorted by the comparison function

  constructor(payload: Component<P, E>[], element: L, compareFn?: (a: P, b: P) => number) {
    super(payload, element)
    this.compareFn = compareFn || ((_a: P, _b: P) => 0);
    this.children = payload;
    this.children.sort((a, b) => this.compareFn(a.payload, b.payload));
    const fragment = document.createDocumentFragment();
    for (const child of this.children)
      fragment.appendChild(child.element);
    this.element.appendChild(fragment);
  }

  override add_child(child: Component<P, E>): void {
    super.add_child(child);
    this.children.push(child);
    this.children.sort((a, b) => this.compareFn(a.payload, b.payload));
    const index = this.children.indexOf(child);
    if (index === this.children.length - 1)
      this.element.appendChild(child.element);
    else
      this.element.insertBefore(child.element, this.children[index + 1].element);
  }

  override remove_child(child: Component<P, E>): void {
    const index = this.children.indexOf(child);
    if (index !== -1) {
      this.children.splice(index, 1);
      super.remove_child(child);
    } else
      throw new Error('Child not found');
  }
}

export class AppComponent extends Component<App, HTMLDivElement> implements AppListener, ConnectionListener {

  constructor() {
    super(App.get_instance(), document.querySelector('#app') as HTMLDivElement);
    this.element.classList.add('d-flex', 'flex-column', 'h-100');

    const fragment = document.createDocumentFragment();

    // Add the Navbar..
    const navbar = document.createElement('nav');
    navbar.classList.add('navbar', 'navbar-expand-lg', 'bg-body-tertiary');
    const nav_container = document.createElement('div');
    nav_container.classList.add('container-fluid');

    const toggler = document.createElement('button');
    toggler.classList.add('navbar-toggler');
    toggler.type = 'button';
    toggler.setAttribute('data-bs-toggle', 'collapse');
    toggler.setAttribute('data-bs-target', '#navbarNav');
    toggler.setAttribute('aria-controls', 'navbarNav');
    toggler.setAttribute('aria-expanded', 'false');
    toggler.setAttribute('aria-label', 'Toggle navigation');
    const toggler_span = document.createElement('span');
    toggler_span.classList.add('navbar-toggler-icon');
    toggler.appendChild(toggler_span);
    nav_container.appendChild(toggler);

    const navbar_collapse = document.createElement('div');
    navbar_collapse.classList.add('collapse', 'navbar-collapse');
    navbar_collapse.id = 'navbarNav';
    this.populate_navbar(navbar_collapse);
    nav_container.appendChild(navbar_collapse);

    navbar.appendChild(nav_container);
    fragment.appendChild(navbar);

    // Add the toast container..
    const toast_container = document.createElement('div');
    toast_container.classList.add('toast-container');
    fragment.appendChild(toast_container);

    this.element.appendChild(fragment);

    App.get_instance().add_app_listener(this);
    Connection.get_instance().add_connection_listener(this);
  }

  toast(info: string): void {
    const toast_container = this.element.querySelector('.toast-container') as HTMLDivElement;
    const toast = document.createElement('div');
    toast.classList.add('toast');
    toast.setAttribute('role', 'alert');
    toast.setAttribute('aria-live', 'assertive');
    toast.setAttribute('aria-atomic', 'true');
    toast.setAttribute('data-bs-autohide', 'true');
    toast.setAttribute('data-bs-delay', '5000');
    const toast_header = document.createElement('div');
    toast_header.classList.add('toast-header');
    const toast_title = document.createElement('strong');
    toast_title.classList.add('me-auto');
    toast_title.innerText = 'Info';
    toast_header.appendChild(toast_title);
    const toast_button = document.createElement('button');
    toast_button.classList.add('btn-close');
    toast_button.setAttribute('type', 'button');
    toast_button.setAttribute('data-bs-dismiss', 'toast');
    toast_button.setAttribute('aria-label', 'Close');
    toast_header.appendChild(toast_button);
    toast.appendChild(toast_header);
    const toast_body = document.createElement('div');
    toast_body.classList.add('toast-body');
    toast_body.innerText = info;
    toast.appendChild(toast_body);
    toast_container.appendChild(toast);
    setTimeout(() => toast.remove(), 5000);
  }

  selected_component(component: Component<any, HTMLElement> | null): void {
    if (component)
      this.add_child(component);
  }

  connected(_info: any): void { }
  received_message(_message: any): void { }
  disconnected(): void { }
  connection_error(_error: any): void { }

  populate_navbar(_container: HTMLDivElement): void { }

  override unmounting(): void {
    App.get_instance().remove_app_listener(this);
    Connection.get_instance().remove_connection_listener(this);
  }
}

export class AnchorComponent<P> extends Component<P, HTMLAnchorElement> {

  constructor(payload: P) { super(payload, document.createElement('a')); }
}

export class ButtonComponent<P> extends Component<P, HTMLButtonElement> {

  constructor(payload: P) { super(payload, document.createElement('button')); }
}

export class UListComponent<P> extends ListComponent<P, HTMLLIElement, HTMLUListElement> {

  constructor(payload: Component<P, HTMLLIElement>[], compareFn?: (a: P, b: P) => number) {
    super(payload, document.createElement('ul'), compareFn);
  }
}