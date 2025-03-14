export class SelectorGroup {

  private selectors = new Set<Selector>();

  constructor() { }

  set_selected(s: Selector) {
    for (const c_s of this.selectors)
      if (c_s != s)
        c_s.unselect();
    s.select();
  }

  add_selector(s: Selector) { this.selectors.add(s); }
  remove_selector(s: Selector) { this.selectors.delete(s); }
}

export interface Selector {

  select(): void;
  unselect(): void;
}