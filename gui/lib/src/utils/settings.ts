export class Settings {

  private protocol: string = 'http';
  private hostname: string = 'localhost';
  private port: number = 80;
  private ws_path: string = '/ws';
  private static instance: Settings;

  private constructor() { }

  public static get_instance(): Settings {
    if (!Settings.instance) {
      Settings.instance = new Settings();
    }
    return Settings.instance;
  }

  public get_protocol(): string { return this.protocol; }
  public get_hostname(): string { return this.hostname; }
  public get_port(): number { return this.port; }
  public get_ws_path(): string { return this.ws_path; }

  public get_host(): string { return this.protocol + '://' + this.hostname + ':' + this.port; }

  public load_settings(settings: any): void {
    this.protocol = settings.protocol || this.protocol;
    this.hostname = settings.host || this.hostname;
    this.port = settings.port || this.port;
    this.ws_path = settings.ws_path || this.ws_path;
  }
}