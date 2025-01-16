import { Settings } from "./settings";

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

  connect(token: string | null = null, timeout = 5000) {
    if (this.socket)
      this.socket.close();

    console.debug('Connecting to server: ', Settings.get_instance().get_host());
    this.socket = new WebSocket(Settings.get_instance().get_host() + '/' + Settings.get_instance().get_ws_path());

    this.socket.onopen = () => {
      console.debug('Connected to server');
      if (token)
        this.socket!.send(JSON.stringify({ type: 'login', token: token }));
      else
        for (const listener of this.connection_listeners) { listener.connected({}); }
    };

    this.socket.onmessage = (event) => {
      console.debug('Received message from server: ', event.data);
      const message = JSON.parse(event.data);
      if (message.type === 'login')
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

  add_connection_listener(listener: ConnectionListener): void { this.connection_listeners.add(listener); }
  remove_connection_listener(listener: ConnectionListener): void { this.connection_listeners.delete(listener); }
}

export interface ConnectionListener {

  connected(info: any): void;

  received_message(message: any): void;

  disconnected(): void;

  connection_error(error: any): void;
}