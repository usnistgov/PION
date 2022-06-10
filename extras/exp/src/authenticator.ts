import byline from "byline";
import Emittery from "emittery";
import { type ExecaChildProcess, execa } from "execa";

interface Events {
  error: Error;
  line: string;
  finish: undefined;
}

/** Control the authenticator program. */
export class Authenticator extends Emittery<Events> {
  private readonly child: ExecaChildProcess<Buffer>;

  /** Start the authenticator. */
  constructor(opts: Authenticator.Options) {
    super();
    this.child = execa("pion-authenticator", [
      "-P", opts.caProfile,
      "-i", "a",
      "-n", opts.deviceName,
      "-N", opts.networkCredential,
      "-p", opts.pakePassword,
    ], {
      buffer: false,
      encoding: null,
      stdin: "ignore",
      stdout: "ignore",
      stderr: "pipe",
      env: {
        NDNPH_UPLINK_UDP: opts.deviceIp,
        NDNPH_UPLINK_UDP_PORT: opts.devicePort.toString(),
        NDNPH_UPLINK_MTU: opts.mtu ? opts.mtu.toString() : undefined,
        NDNPH_KEYCHAIN: opts.keychain,
      },
    });
    void this.child.on("error", (err) => {
      void this.emit("error", err);
    });
    void this.handleStderr();
  }

  public readonly logs: string[] = [];

  private async handleStderr() {
    for await (const line of byline(this.child.stderr!, { encoding: "utf8" }) as AsyncIterable<string>) {
      this.logs.push(line);
      void this.emit("line", line);
    }
    void this.emit("finish");
  }

  /** Kill the authenticator. */
  public close(): void {
    this.child.kill();
  }

  public get result(): Authenticator.Result {
    return {
      logs: this.logs,
    };
  }
}

export namespace Authenticator {
  export interface Options {
    deviceIp: string;
    devicePort: number;
    mtu?: number;
    keychain: string;

    caProfile: string;
    deviceName: string;
    networkCredential: string;
    pakePassword: string;
  }

  export interface Result {
    logs: string[];
  }
}
