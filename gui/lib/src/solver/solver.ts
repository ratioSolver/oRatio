export namespace solver {

  class Env {

    exprs: Map<string, values.Value> = new Map();

    get_exprs(): Map<string, values.Value> { return this.exprs; }

    to_string(items: Map<number, values.Value>, expressive: boolean): string {
      return `{${Array.from(this.exprs.entries()).map(([name, value]) => `${name}: ${value.to_string(items, expressive)}`).join(', ')}}`;
    }
  }

  export enum SolverState {
    reasoning,
    idle,
    adapting,
    executing,
    finished,
    failed
  }

  export class Solver extends Env implements SolverListener {

    private id: number;
    private name: string;
    private state: SolverState;

    private items: Map<number, values.Item> = new Map();
    private atoms: Map<number, values.Atom> = new Map();
    private flaws: Map<number, graph.Flaw> = new Map();
    private resolvers: Map<number, graph.Resolver> = new Map();
    private c_flaw: graph.Flaw | null = null;
    private c_resolver: graph.Resolver | null = null;
    private solver_listeners: Set<SolverListener> = new Set();

    constructor(id: number, name: string, state: SolverState) {
      super();
      this.id = id;
      this.name = name;
      this.state = state;
    }

    get_id(): number { return this.id; }
    get_name(): string { return this.name; }
    get_state(): SolverState { return this.state; }
    get_flaws(): Map<number, graph.Flaw> { return this.flaws; }
    get_flaw(id: number): graph.Flaw { return this.flaws.get(id)!; }
    get_resolvers(): Map<number, graph.Resolver> { return this.resolvers; }
    get_resolver(id: number): graph.Resolver { return this.resolvers.get(id)!; }
    get_current_flaw(): graph.Flaw | null { return this.c_flaw; }
    get_current_resolver(): graph.Resolver | null { return this.c_resolver; }

    state_changed(state: SolverState): void {
      this.state = state;
      for (const listener of this.solver_listeners) listener.state_changed(state);
    }
    flaw_created(flaw: graph.Flaw): void {
      this.flaws.set(flaw.get_id(), flaw);
      for (const listener of this.solver_listeners) listener.flaw_created(flaw);
    }
    flaw_cost_changed(flaw: graph.Flaw): void {
      this.flaws.set(flaw.get_id(), flaw);
      for (const listener of this.solver_listeners) listener.flaw_cost_changed(flaw);
    }
    current_flaw(flaw: graph.Flaw | null): void {
      this.c_flaw = flaw;
      if (this.c_resolver)
        this.current_resolver(null);
      for (const listener of this.solver_listeners) listener.current_flaw(flaw);
    }
    resolver_created(resolver: graph.Resolver): void {
      this.resolvers.set(resolver.get_id(), resolver);
      for (const listener of this.solver_listeners) listener.resolver_created(resolver);
    }
    current_resolver(resolver: graph.Resolver | null): void {
      this.c_resolver = resolver;
      for (const listener of this.solver_listeners) listener.current_resolver(resolver);
    }

    add_solver_listener(listener: SolverListener) { this.solver_listeners.add(listener); }
    remove_solver_listener(listener: SolverListener) { this.solver_listeners.delete(listener); }

    static make_solver(solver_message: SolverMessage): Solver {
      const solver = new Solver(solver_message.id, solver_message.name, SolverState[solver_message.state as keyof typeof SolverState]);

      if (solver_message.items) // we create the items..
        for (const [id, im] of solver_message.items)
          solver.items.set(Number(id), new values.Item(Number(id), im.type, im.name));
      if (solver_message.atoms) // we create the atoms..
        for (const [id, am] of solver_message.atoms)
          solver.atoms.set(Number(id), new values.Atom(Number(id), am.type, am.name, am.fact, am.sigma, values.AtomState[am.state as keyof typeof values.AtomState]));

      if (solver_message.items) // we set the exprs for the items..
        for (const [id, im] of solver_message.items)
          if (im.exprs)
            for (const [name, expr] of im.exprs)
              solver.items.get(Number(id))!.exprs.set(name, values.make_value(expr, solver.items, solver.atoms));
      if (solver_message.atoms) // we set the exprs for the atoms..
        for (const [id, am] of solver_message.atoms)
          if (am.exprs)
            for (const [name, expr] of am.exprs)
              solver.atoms.get(Number(id))!.exprs.set(name, values.make_value(expr, solver.items, solver.atoms));

      if (solver_message.exprs) // we set the exprs for the solver..
        for (const [name, expr] of solver_message.exprs)
          solver.exprs.set(name, values.make_value(expr, solver.items, solver.atoms));

      return solver;
    }
  }

  export interface SolverListener {

    state_changed(state: SolverState): void;

    flaw_created(flaw: graph.Flaw): void;
    flaw_cost_changed(flaw: graph.Flaw): void;
    current_flaw(flaw: graph.Flaw | null): void;

    resolver_created(resolver: graph.Resolver): void;
    current_resolver(resolver: graph.Resolver | null): void;
  }

  export class SolverSet implements SolverSetListener {

    private static instance: SolverSet;
    private solvers: Map<number, solver.Solver> = new Map();
    private solver_set_listeners: Set<SolverSetListener> = new Set();

    private constructor() { }

    static get_instance() {
      if (!SolverSet.instance)
        SolverSet.instance = new SolverSet();
      return SolverSet.instance;
    }

    init(solvers: Map<number, Solver>): void {
      this.solvers = solvers;
      for (const listener of this.solver_set_listeners) { listener.init(solvers); }
    }

    solver_created(solver: Solver): void {
      this.solvers.set(solver.get_id(), solver);
      for (const listener of this.solver_set_listeners) { listener.solver_created(solver); }
    }

    solver_deleted(id: number): void {
      this.solvers.delete(id);
      for (const listener of this.solver_set_listeners) { listener.solver_deleted(id); }
    }

    update_solvers(message: any): void {
      switch (message.type) {
        case 'solvers':
          const solvers = new Map<number, Solver>();
          for (const solver_message of message.solvers)
            solvers.set(solver_message.id, Solver.make_solver(solver_message));
          this.init(solvers);
          break;
        case 'new_solver':
          this.solver_created(Solver.make_solver(message.solver));
          break;
        case 'deleted_solver':
          this.solver_deleted(message.id);
          break;
        case 'flaw_created':
          const flaw_message: FlawMessage = message.flaw;
          const resolvers: graph.Resolver[] = flaw_message.causes.map((id: number) => this.solvers.get(flaw_message.solver_id)!.get_resolver(id));
          this.solvers.get(flaw_message.solver_id)!.flaw_created(new graph.Flaw(flaw_message.id, flaw_message.phi, resolvers, graph.State[flaw_message.state as keyof typeof graph.State], flaw_message.cost, flaw_message.data));
          break;
        case 'flaw_state_changed':
          break;
        case 'flaw_cost_changed':
          break;
        case 'flaw_position_changed':
          break;
        case 'current_flaw':
          break;
        case 'resolver_created':
          break;
        case 'resolver_state_changed':
          break;
        case 'current_resolver':
          break;
        case 'causal_link_added':
          break;
        case 'solver_execution_state_changed':
          break;
        case 'tick':
          break;
        case 'starting':
          break;
        case 'ending':
          break;
        case 'start':
          break;
        case 'end':
          break;
      }
    }

    add_solver_set_listener(listener: SolverSetListener) { this.solver_set_listeners.add(listener); }
    remove_solver_set_listener(listener: SolverSetListener) { this.solver_set_listeners.delete(listener); }
  }

  export interface SolverSetListener {

    init(solvers: Map<number, Solver>): void;

    solver_created(solver: Solver): void;

    solver_deleted(id: number): void;
  }

  export namespace graph {
    /**
     * Represents the state of a flaw or resolver in the CoCo framework.
     */
    export enum State {
      active,
      inactive,
      forbidden
    }

    export class Flaw {

      private id: number;
      private phi: string;
      private causes: Resolver[];
      private state: State;
      private cost: number;
      private data: FlawData;

      constructor(id: number, phi: string, causes: Resolver[], state: State, cost: number, data: FlawData) {
        this.id = id;
        this.phi = phi;
        this.causes = causes;
        this.state = state;
        this.cost = cost;
        this.data = data;
      }

      get_id(): number { return this.id; }
      get_phi(): string { return this.phi; }
      get_causes(): Resolver[] { return this.causes; }
      get_state(): State { return this.state; }
      get_cost(): number { return this.cost; }

      to_string(expressive = false): string {
        if (expressive)
          switch (this.data.type) {
            case 'atom':
              return this.phi + ' ' + (this.data.atom!.is_fact ? 'fact' : 'goal') + ' ' + this.data.atom!.type.split(':').pop() + ' ' + this.cost;
            default:
              return this.phi;
          }
        else
          switch (this.data.type) {
            case 'atom':
              return (this.data.atom!.is_fact ? 'fact' : 'goal') + ' ' + this.data.atom!.type.split(':').pop();
            default:
              return this.phi;
          }
      }
    }

    export class Resolver {

      private id: number;
      private rho: string;
      private preconditions: Flaw[];
      private flaw: Flaw;
      private state: State;
      private intrinsic_cost: number;
      private data: ResolverData;

      constructor(id: number, rho: string, preconditions: Flaw[], flaw: Flaw, state: State, intrinsic_cost: number, data: ResolverData) {
        this.id = id;
        this.rho = rho;
        this.preconditions = preconditions;
        this.flaw = flaw;
        this.state = state;
        this.intrinsic_cost = intrinsic_cost;
        this.data = data;
      }

      get_id(): number { return this.id; }
      get_rho(): string { return this.rho; }
      get_preconditions(): Flaw[] { return this.preconditions; }
      get_flaw(): Flaw { return this.flaw; }
      get_state(): State { return this.state; }
      get_intrinsic_cost(): number { return this.intrinsic_cost; }

      get_cost(): number {
        if (this.state == State.forbidden)
          return Infinity;
        return (this.preconditions.length ? Math.max.apply(null, this.preconditions.map(flaw => flaw.get_cost())) : 0) + this.intrinsic_cost;
      }

      to_string(expressive = false): string {
        if (expressive)
          switch (this.data.type) {
            case 'activate_fact':
            case 'activate_goal':
              return this.rho + ' activate ' + this.get_cost();
            case 'unify_atom':
              return this.rho + ' unify ' + this.get_cost();
            default:
              return this.rho + ' ' + this.get_cost();
          }
        else
          switch (this.data.type) {
            case 'activate_fact':
            case 'activate_goal':
              return 'activate';
            case 'unify_atom':
              return 'unify';
            default:
              return this.rho;
          }
      }
    }
  }

  export namespace values {

    export class Rational {

      private num: number;
      private den: number;

      constructor(num: number, den: number) {
        this.num = num;
        this.den = den;
      }

      to_number(): number { return this.num / this.den; }

      to_string(): string {
        switch (this.den) {
          case 1:
            return this.num.toString();
          case 0:
            return this.num > 0 ? '+∞' : '-∞';
          default:
            return `${this.num}/${this.den}`;
        }
      }

      static make_rational(val: RationalMessage): Rational { return new Rational(val.num, val.den); }
    }

    export class InfRational extends Rational {

      private inf: Rational;

      constructor(num: number, den: number, inf: Rational = new Rational(0, 1)) {
        super(num, den);
        this.inf = inf;
      }

      static make_inf_rational(val: InfRationalMessage): InfRational { return new InfRational(val.num, val.den, val.inf ? Rational.make_rational(val.inf) : new Rational(0, 1)); }
    }

    export interface Value {

      to_string(items: Map<number, Value>, expressive: boolean): string;
    }

    export enum LBool {

      True,
      False,
      Undefined
    }

    export class Bool implements Value {

      lit: string;
      val: LBool;

      constructor(lit: string, val: LBool) {
        this.lit = lit;
        this.val = val;
      }

      to_string(_: Map<number, Value>, expressive = false): string {
        switch (this.val) {
          case LBool.True:
            return expressive ? 'true' : '⊤';
          case LBool.False:
            return expressive ? 'false' : '⊥';
          default:
            return expressive ? 'undefined' : 'U';
        }
      }

      static make_bool(val: BoolMessage): Bool { return new Bool(val.lit, LBool[val.val as keyof typeof LBool]); }
    }

    export class Int implements Value {

      lin: string;
      val: number;
      lb?: number;
      ub?: number;

      constructor(lin: string, val: number, lb?: number, ub?: number) {
        this.lin = lin;
        this.val = val;
        this.lb = lb;
        this.ub = ub;
      }

      to_string(_: Map<number, Value>, expressive = false): string {
        if (expressive) {
          let res = `${this.val}`;
          if (this.lb || this.ub)
            res += `[${this.lb ?? '-∞'}, ${this.ub ?? '+∞'}]`;
          return res + `(${this.lin})`;
        } else
          return this.val.toString();
      }

      static make_int(val: IntMessage): Int { return new Int(val.lin, val.val, val.lb, val.ub); }
    }

    export class Real implements Value {

      lin: string;
      val: InfRational;
      lb?: InfRational;
      ub?: InfRational;

      constructor(lin: string, val: InfRational, lb?: InfRational, ub?: InfRational) {
        this.lin = lin;
        this.val = val;
        this.lb = lb;
        this.ub = ub;
      }

      to_string(_: Map<number, Value>, expressive = false): string {
        if (expressive) {
          let res = `${this.val.to_string()}`;
          if (this.lb || this.ub)
            res += `[${this.lb?.to_string() ?? '-∞'}, ${this.ub?.to_string() ?? '+∞'}]`;
          return res + `(${this.lin})`;
        } else
          return this.val.to_string();
      }

      static make_real(val: RealMessage): Real { return new Real(val.lin, InfRational.make_inf_rational(val.val), val.lb ? InfRational.make_inf_rational(val.lb!) : undefined, val.ub ? InfRational.make_inf_rational(val.ub!) : undefined); }
    }

    export class Time implements Value {

      lin: string;
      val: InfRational;
      lb?: InfRational;
      ub?: InfRational;

      constructor(lin: string, val: InfRational, lb?: InfRational, ub?: InfRational) {
        this.lin = lin;
        this.val = val;
        this.lb = lb;
        this.ub = ub;
      }

      to_string(_: Map<number, Value>, expressive = false): string {
        if (expressive) {
          let res = `${this.val.to_string()}`;
          if (this.lb || this.ub)
            res += `[${this.lb?.to_string() ?? '-∞'}, ${this.ub?.to_string() ?? '+∞'}]`;
          return res + `(${this.lin})`;
        } else
          return this.val.to_string();
      }

      static make_time(val: TimeMessage): Time { return new Time(val.lin, InfRational.make_inf_rational(val.val), val.lb ? InfRational.make_inf_rational(val.lb!) : undefined, val.ub ? InfRational.make_inf_rational(val.ub!) : undefined); }
    }

    export class String implements Value {

      val: string;

      constructor(val: string) {
        this.val = val;
      }

      to_string(items: Map<number, Value>, expressive = false): string {
        const num = Number(this.val);
        if (typeof num === "number" && !isNaN(num) && items.has(num))
          return items.get(num)!.to_string(items, expressive);
        else
          return `'${this.val}'`;
      }

      static make_string(val: StringMessage): String { return new String(val.val); }
    }

    export class Enum implements Value {

      var: string;
      vals: Item[];

      constructor(v: string, vals: Item[]) {
        this.var = v;
        this.vals = vals;
      }

      to_string(items: Map<number, Value>, expressive = false): string {
        if (expressive)
          return (this.vals.length == 1 ? this.vals[0].to_string(items, expressive) : `{${this.vals.map((item: Item) => item.to_string(items, expressive)).join(', ')}}`) + ` (${this.var})`;
        else
          return this.vals.length == 1 ? this.vals[0].to_string(items, expressive) : `{${this.vals.map((item: Item) => item.to_string(items, expressive)).join(', ')}}`;
      }

      static make_enum(val: EnumMessage, items: Map<number, Value>): Enum { return new Enum(val.var, val.vals.map((item: number) => items.get(item) as Item)); }
    }

    export class Item extends Env implements Value {

      private id: number;
      protected type: string;
      private name: string;

      constructor(id: number, type: string, name: string) {
        super();
        this.id = id;
        this.type = type;
        this.name = name;
      }

      get_id(): number { return this.id; }
      get_type(): string { return this.type; }
      get_name(): string { return this.name; }

      to_string(items: Map<number, Value>, expressive = false): string {
        if (expressive)
          return this.type.split(':').pop() + ' ' + this.name + super.to_string(items, expressive);
        else
          return this.name;
      }
    }

    export enum AtomState {
      Active,
      Inactive,
      Unified
    }

    export class Atom extends Item implements Value {

      private fact: boolean;
      private sigma: string;
      private state: AtomState;

      constructor(id: number, type: string, name: string, fact: boolean, sigma: string, state: AtomState) {
        super(id, type, name);
        this.fact = fact;
        this.sigma = sigma;
        this.state = state;
      }

      is_fact(): boolean { return this.fact; }
      get_sigma(): string { return this.sigma; }
      get_state(): AtomState { return this.state; }

      to_string(items: Map<number, Value>, expressive = false): string {
        let pars = Array.from(this.exprs.entries());
        if (!expressive)
          pars = pars.filter(([name, _]) => name !== 'start' && name !== 'end' && name !== 'duration' && name !== 'tau');
        const pars_str = pars.map(([name, value]) => `${name}: ${value.to_string(items, expressive)}`).join(', ');
        if (expressive)
          return this.sigma + ' ' + this.type.split(':').pop() + `(${pars_str})`;
        else
          return this.type.split(':').pop() + `(${pars_str})`;
      }
    }

    export function make_value(value_message: ValueMessage, items: Map<number, Value>, atoms: Map<number, Atom>): Value {
      switch (value_message.type) {
        case 'bool':
          return Bool.make_bool(value_message as BoolMessage);
        case 'int':
          return Int.make_int(value_message as IntMessage);
        case 'real':
          return Real.make_real(value_message as RealMessage);
        case 'time':
          return Time.make_time(value_message as TimeMessage);
        case 'string':
          return String.make_string(value_message as StringMessage);
        case 'enum':
          return Enum.make_enum(value_message as EnumMessage, items);
        case 'item':
          return items.get((value_message as ItemValueMessage).val)!;
        case 'atom':
          return atoms.get((value_message as AtomValueMessage).val)!;
        default:
          throw new Error(`Unknown type: ${value_message.type}`);
      }
    }
  }

  export namespace timelines {

    type Impulse = { at: values.InfRational };
    type Interval = { from: values.InfRational, to: values.InfRational };
    export type TimelineValue = Impulse | Interval;

    export class Timeline<V extends TimelineValue> {

      private id: string;
      private name: string;
      private values: V[];

      constructor(id: string, name: string, values: V[]) {
        this.id = id;
        this.name = name;
        this.values = values;
      }

      public get_name(): string { return this.name; }

      public get_values(): V[] { return this.values; }
    }

    type SolverTimelineValue = values.Atom & (Impulse | Interval);

    export class SolverTimeline extends Timeline<SolverTimelineValue> {

      constructor(id: string, name: string, values: SolverTimelineValue[]) {
        super(id, name, values);
      }
    }

    type AgentTimelineValue = values.Atom & (Impulse | Interval);

    export class AgentTimeline extends Timeline<AgentTimelineValue> {

      constructor(id: string, name: string, values: AgentTimelineValue[]) {
        super(id, name, values);
      }
    }

    type StateVariableTimelineValue = { atoms: values.Atom[] } & Interval;

    export class StateVariableTimeline extends Timeline<StateVariableTimelineValue> {

      constructor(id: string, name: string, values: StateVariableTimelineValue[]) {
        super(id, name, values);
      }

      static to_string(items: Map<number, values.Value>, value: StateVariableTimelineValue, expressive = false): string {
        if (expressive)
          switch (value.atoms.length) {
            case 0:
              return `[] (${value.from.to_string()} - ${value.to.to_string()})`;
            case 1:
              return value.atoms[0].to_string(items, expressive);
            default:
              return `[${value.atoms.map(atom => atom.to_string(items, expressive)).join(', ')}] (${value.from.to_string()} - ${value.to.to_string()})`;
          }
        else
          switch (value.atoms.length) {
            case 0:
              return '[]';
            case 1:
              return value.atoms[0].to_string(items, expressive);
            default:
              return `[${value.atoms.map(atom => atom.to_string(items, expressive)).join(', ')}]`;
          }
      }
    }

    type ReusableResourceTimelineValue = { atoms: values.Atom[], usage: values.Rational } & Interval;

    export class ReusableResourceTimeline extends Timeline<ReusableResourceTimelineValue> {

      capacity: values.InfRational;

      constructor(id: string, name: string, capacity: values.InfRational, values: ReusableResourceTimelineValue[]) {
        super(id, name, values);
        this.capacity = capacity;
      }

      static to_string(items: Map<number, values.Value>, value: ReusableResourceTimelineValue, expressive = false): string {
        if (expressive)
          switch (value.atoms.length) {
            case 0:
              return `0 (${value.from.to_string()} - ${value.to.to_string()})`;
            case 1:
              return value.usage.to_string() + ' ' + value.atoms[0].to_string(items, expressive);
            default:
              return value.usage.to_string() + ` {${value.atoms.map(atom => atom.to_string(items, expressive)).join(', ')}} (${value.from.to_string()} - ${value.to.to_string()})`;
          }
        else
          return value.usage.to_string();
      }
    }

    type ConsumableResourceTimelineValue = { atoms: values.Atom[], start: values.InfRational, end: values.InfRational } & Interval;

    export class ConsumableResourceTimeline extends Timeline<ConsumableResourceTimelineValue> {

      capacity: values.InfRational;
      initial_amount: values.InfRational;

      constructor(id: string, name: string, capacity: values.InfRational, initial_amount: values.InfRational, values: ConsumableResourceTimelineValue[]) {
        super(id, name, values);
        this.capacity = capacity;
        this.initial_amount = initial_amount;
      }

      static to_string(items: Map<number, values.Value>, value: ConsumableResourceTimelineValue, expressive = false): string {
        if (expressive)
          switch (value.atoms.length) {
            case 0:
              return `- (${value.from.to_string()} - ${value.to.to_string()})`;
            case 1:
              return `${value.start.to_string()} - ${value.end.to_string()} ${value.atoms[0].to_string(items, expressive)}`;
            default:
              return `${value.start.to_string()} - ${value.end.to_string()} {${value.atoms.map(atom => atom.to_string(items, expressive)).join(', ')}} (${value.from.to_string()} - ${value.to.to_string()})`;
          }
        else
          switch (value.atoms.length) {
            case 0:
              return '-';
            default:
              return `${value.start.to_string()} - ${value.end.to_string()}`;
          }
      }
    }

    export function make_timeline(timeline: any, atoms: Map<number, values.Atom>): Timeline<TimelineValue> {
      switch (timeline.type) {
        case 'Solver':
          return new SolverTimeline(timeline.id, timeline.name, timeline.values.map((value: number) => atoms.get(value)));
        case 'Agent':
          return new AgentTimeline(timeline.id, timeline.name, timeline.values.map((value: number) => atoms.get(value)));
        case 'StateVariable':
          return new StateVariableTimeline(timeline.id, timeline.name, timeline.values.map((value: any) => { return { from: values.InfRational.make_inf_rational(value.from), to: values.InfRational.make_inf_rational(value.to), atoms: value.atoms.map((atom: any) => atoms.get(atom)) }; }));
        case 'ReusableResource':
          return new ReusableResourceTimeline(timeline.id, timeline.name, values.InfRational.make_inf_rational(timeline.capacity), timeline.values.map((value: any) => { return { from: values.InfRational.make_inf_rational(value.from), to: values.InfRational.make_inf_rational(value.to), atoms: value.atoms.map((atom: any) => atoms.get(atom)), usage: values.InfRational.make_inf_rational(value.usage) }; }));
        case 'ConsumableResource':
          return new ConsumableResourceTimeline(timeline.id, timeline.name, values.InfRational.make_inf_rational(timeline.capacity), values.InfRational.make_inf_rational(timeline.initial_amount), timeline.values.map((value: any) => { return { from: values.InfRational.make_inf_rational(value.from), to: values.InfRational.make_inf_rational(value.to), atoms: value.atoms.map((atom: any) => atoms.get(atom)), start: values.InfRational.make_inf_rational(value.start), end: values.InfRational.make_inf_rational(value.end) }; }));
        default:
          throw new Error(`Unknown timeline type: ${timeline.type}`);
      }
    }
  }
}

interface SolverMessage {

  id: number;
  name: string;
  state: string;
  items?: Map<number, ItemMessage>;
  atoms?: Map<number, AtomMessage>;
  exprs?: Map<string, ValueMessage>;
}

interface RationalMessage {

  num: number;
  den: number;
}

interface InfRationalMessage extends RationalMessage {

  inf?: RationalMessage;
}

interface ItemMessage {

  type: string;
  name: string;
  exprs?: Map<string, ValueMessage>;
}

interface AtomMessage extends ItemMessage {

  fact: boolean;
  sigma: string;
  state: string;
}
interface BoolMessage {

  type: string;
  lit: string;
  val: string;
}

interface IntMessage {

  type: string;
  lin: string;
  val: number;
  lb?: number;
  ub?: number;
}

interface RealMessage {

  type: string;
  lin: string;
  val: InfRationalMessage;
  lb?: InfRationalMessage;
  ub?: InfRationalMessage;
}

interface TimeMessage {

  type: string;
  lin: string;
  val: InfRationalMessage;
  lb?: InfRationalMessage;
  ub?: InfRationalMessage;
}

interface StringMessage {

  type: string;
  val: string;
}

interface EnumMessage {

  type: string;
  var: string;
  vals: number[];
}

interface ItemValueMessage {

  type: string;
  val: number;
}

interface AtomValueMessage {

  type: string;
  val: number;
}

type ValueMessage = BoolMessage | IntMessage | RealMessage | TimeMessage | StringMessage | EnumMessage | ItemValueMessage | AtomValueMessage;

interface FlawMessage {

  solver_id: number;
  id: number;
  phi: string;
  causes: number[];
  state: string;
  cost: number;
  data: {
    type: string;
    atom?: {
      sigma: number;
      type: string;
      is_fact: boolean;
    };
  };
}

interface FlawData {

  type: string;
  atom?: {
    sigma: number;
    type: string;
    is_fact: boolean;
  };
}

interface ResolverMessage {

  id: number;
  rho: string;
  preconditions: number[];
  flaw: number;
  state: string;
  intrinsic_cost: number;
  data: ResolverData;
}

interface ResolverData {

  type: string;
  name?: string;
  value?: any;
}