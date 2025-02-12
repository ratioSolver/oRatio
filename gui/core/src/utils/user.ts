import { Settings } from "./settings";
import { Connection } from './connection';

export class User {

  username: string;

  constructor(username: string) {
    this.username = username;
  }
}

/**
 * Singleton class that manages the current user.
 */
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

  /**
   * Logs in a user.
   * 
   * @param username Username.
   * @param password Password.
   * @param remember_input Whether to remember the input.
   * @returns Whether the login was successful.
   */
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

  /**
   * Gets the current user.
   * 
   * @returns Current user.
   * @returns null if no user is connected.
   */
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

  /**
   * Adds a user listener.
   * 
   * @param listener Listener to add.
   */
  add_user_listener(listener: UserListener): void {
    this.user_listeners.add(listener);
  }

  /**
   * Removes a user listener.
   * 
   * @param listener Listener to remove.
   */
  remove_user_listener(listener: UserListener): void {
    this.user_listeners.delete(listener);
  }
}

/**
 * Listener for user events.
 */
export interface UserListener {

  /**
   * Called when a user is connected.
   * 
   * @param user User that is connected.
   */
  connected(user: User): void;

  /**
   * Called when a user is logged out.
   */
  logged_out(): void;
}