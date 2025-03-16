import { Settings } from "./settings";

/**
 * Singleton class that manages the connection to the server.
 */
export class Connection {

  private static instance: Connection;
  private socket: WebSocket | null = null;
  private connection_listeners: Set<ConnectionListener> = new Set();

  private constructor() { }

  static get_instance() {
    if (!Connection.instance)
      Connection.instance = new Connection();
    return Connection.instance;
  }

  /**
   * Connects to the server.
   * 
   * @param token Token to use for authentication.
   * @param timeout Timeout in milliseconds to wait before reconnecting.
   */
  connect(token: string | null = null, timeout = 5000) {
    if (this.socket)
      this.socket.close();

    console.debug('Connecting to server: ', Settings.get_instance().get_host());
    this.socket = new WebSocket(Settings.get_instance().get_host() + '/' + Settings.get_instance().get_ws_path());

    this.socket.onopen = () => {
      console.debug('Connected to server');
      if (token)
        this.socket!.send(JSON.stringify({ msg_type: 'login', token: token }));
      else
        for (const listener of this.connection_listeners) { listener.connected({}); }
    };

    this.socket.onmessage = (event) => {
      console.debug('Received message from server: ', event.data);
      const message = JSON.parse(event.data);
      if (message.msg_type === 'login')
        for (const listener of this.connection_listeners) { listener.connected(message.info); }
      else
        for (const listener of this.connection_listeners) { listener.received_message(message); }
    };

    this.socket.onclose = () => {
      console.debug('Disconnected from server');
      for (const listener of this.connection_listeners) { listener.disconnected(); }
    };

    this.socket.onerror = (error) => {
      console.error('Connection error: ', error);
      for (const listener of this.connection_listeners) { listener.connection_error(error); }
      setTimeout(() => this.connect(token, timeout), timeout);
    };
  }

  /**
   * Adds a connection listener.
   * 
   * @param listener Connection listener to add.
   */
  add_connection_listener(listener: ConnectionListener): void { this.connection_listeners.add(listener); }

  /**
   * Removes a connection listener.
   * 
   * @param listener Connection listener to remove.
   */
  remove_connection_listener(listener: ConnectionListener): void { this.connection_listeners.delete(listener); }
}

/**
 * Interface that defines the methods that a connection listener must implement.
 */
export interface ConnectionListener {

  /**
   * Called when the connection to the server is established.
   *
   * @param info Information received from the server.
   */
  connected(info: any): void;

  /**
   * Called when a message is received from the server.
   *
   * @param message Message received from the server.
   */
  received_message(message: any): void;

  /**
   * Called when the connection to the server is closed.
   */
  disconnected(): void;

  /**
   * Called when an error occurs in the connection to the server.
   *
   * @param error Error that occurred.
   */
  connection_error(error: any): void;
}