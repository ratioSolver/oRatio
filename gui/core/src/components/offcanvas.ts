import { App, Component } from "../app";

export class Offcanvas extends Component<App, HTMLDivElement> {

  constructor(id: string, top: Map<HTMLAnchorElement, Component<any, HTMLElement>>, bottom?: Map<HTMLAnchorElement, Component<any, HTMLElement>>) {
    super(App.get_instance(), document.createElement('div'));

    this.element.classList.add('offcanvas', 'offcanvas-start', 'd-flex');
    this.element.tabIndex = -1;
    this.element.id = id;

    const body = document.createElement('div');
    body.classList.add('offcanvas-body', 'flex-column', 'flex-shrink-0', 'p-3', 'bg-light');

    const ul = document.createElement('ul');
    ul.classList.add('nav', 'nav-pills', 'flex-column', 'mb-auto');

    for (const [a, comp] of top) {
      const li = document.createElement('li');
      li.classList.add('nav-item');

      a.classList.add('nav-link');
      a.onclick = () => {
        for (const oa of top.keys())
          oa.classList.remove('active');
        if (bottom)
          for (const oa of bottom.keys())
            oa.classList.remove('active');
        a.classList.add('active');
        App.get_instance().selected_component(comp);
      };

      li.appendChild(a);
      ul.appendChild(li);
    }

    body.appendChild(ul);

    if (bottom) {
      const hr = document.createElement('hr');
      body.appendChild(hr);

      const footer_ul = document.createElement('ul');
      footer_ul.classList.add('nav', 'nav-pills', 'flex-column', 'mb-0');

      for (const [a, comp] of bottom) {
        const li = document.createElement('li');
        li.classList.add('nav-item');

        a.classList.add('nav-link');
        a.onclick = () => {
          for (const oa of top.keys())
            oa.classList.remove('active');
          for (const oa of bottom.keys())
            oa.classList.remove('active');
          a.classList.add('active');
          App.get_instance().selected_component(comp);
        };

        li.appendChild(a);
        footer_ul.appendChild(li);
      }

      body.appendChild(footer_ul);
    }

    this.element.append(body);
  }
}