export interface DirectConnection {
  connect(): Promise<void>;
  disconnect(): Promise<void>;
}
