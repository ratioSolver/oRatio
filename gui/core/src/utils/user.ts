import { Settings } from "./settings";
import { Connection } from './connection';

export class User {

  username: string;

  constructor(username: string) {
    this.username = username;
  }
}

export class CurrentUser implements UserListener {

  private static instance: CurrentUser;
  private user: User | null = null;
  private user_listeners: Set<UserListener> = new Set();

  private constructor() { }

  static get_instance() {
    if (!CurrentUser.instance)
      CurrentUser.instance = new CurrentUser();
    return CurrentUser.instance;
  }

  async login(username: string, password: string, remember_input: boolean = false): Promise<boolean> {
    const response = await fetch(Settings.get_instance().get_host() + '/login', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ username: username, password: password }) });
    if (response.ok) { // Login successful
      const data = await response.json();
      if (remember_input)
        localStorage.setItem('token', data.token);
      Connection.get_instance().connect(data.token);
      return true;
    } else { // Login failed
      return false;
    }
  }

  get_user(): User | null { return this.user; }

  connected(user: User): void {
    this.user = user;
    for (const listener of this.user_listeners) { listener.connected(user); }
  }

  logged_out(): void {
    this.user = null;
    localStorage.removeItem('token');
    for (const listener of this.user_listeners) { listener.logged_out(); }
  }

  add_user_listener(listener: UserListener): void {
    this.user_listeners.add(listener);
  }

  remove_user_listener(listener: UserListener): void {
    this.user_listeners.delete(listener);
  }
}

export interface UserListener {

  connected(user: User): void;

  logged_out(): void;
}