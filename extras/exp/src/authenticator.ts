import byline from "byline";
import Emittery from "emittery";
import execa, { ExecaChildProcess } from "execa";

interface Events {
  error: Error;
  line: string;
  finish: undefined;
}

export class Authenticator extends Emittery<Events> {
  private readonly child: ExecaChildProcess<Buffer>;

  constructor(opts: Authenticator.Options) {
    super();
    this.child = execa("ndnob-authenticator", [
      "-P", opts.caProfile,
      "-i", "a",
      "-n", opts.deviceName,
      "-N", opts.networkCredential,
      "-p", opts.pakePassword,
    ], {
      buffer: false,
      encoding: null,
      stdin: "ignore",
      stdout: "pipe",
      stderr: "inherit",
      env: {
        NDNPH_UPLINK_UDP: opts.deviceIp,
        NDNPH_UPLINK_UDP_PORT: opts.devicePort.toString(),
        NDNPH_UPLINK_MTU: opts.mtu ? opts.mtu.toString() : undefined,
        NDNPH_KEYCHAIN: opts.keychain,
      },
    });
    // eslint-disable-next-line @typescript-eslint/no-floating-promises
    this.child.on("error", (err) => {
      void this.emit("error", err);
    });
    void this.handleStdout();
  }

  public readonly output: string[] = [];

  private async handleStdout() {
    for await (const line of byline(this.child.stdout!, { encoding: "utf-8" }) as AsyncIterable<string>) {
      this.output.push(line);
      void this.emit("line", line);
    }
    void this.emit("finish");
  }

  public close(): void {
    this.child.kill();
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
}
