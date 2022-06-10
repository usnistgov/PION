import { type ExecaChildProcess, execa } from "execa";

import type { DirectConnection } from "./direct-connection";

/** Control the Bluetooth Low Energy bridge script. */
export class BleBridge implements DirectConnection {
  private connected = false;
  private child?: ExecaChildProcess<Buffer>;
  private finish?: Promise<void>;

  constructor(private readonly addr: string, private readonly bridgePath: string) {}

  private async loop(): Promise<void> {
    while (this.connected) {
      try {
        this.child = execa("pipenv", [
          "run", "python", "BleUdpBridge.py",
          "--addr", this.addr,
          "--listen-addr", BleBridge.IP,
          "--listen-port", `${BleBridge.PORT}`,
        ], {
          encoding: null,
          stdin: "ignore",
          stdout: "ignore",
          stderr: "inherit",
          cwd: this.bridgePath,
        });
        await this.child;
      } catch {} finally {
        this.child?.kill();
      }
    }
  }

  public async connect() {
    this.connected = true;
    this.finish = this.loop();
  }

  public async disconnect() {
    this.connected = false;
    this.child?.kill();
    await this.finish;
  }
}

export namespace BleBridge {
  export const IP = "127.0.0.1";
  export const PORT = 6362;
}
