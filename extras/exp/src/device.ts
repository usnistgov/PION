import byline from "byline";
import Emittery from "emittery";
import execa, { ExecaChildProcess } from "execa";

export enum AppState {
  Idle,
  MakePassword,
  WaitDirectConnect,
  WaitPake,
  WaitDirectDisconnect,
  WaitInfraConnect,
  WaitNdncert,
  Success,
  Failure,
  Final,
}

class DeviceLogLine {
  constructor(line: string) {
    const tokens = line.split(" ");
    if (tokens.length < 4) {
      throw new Error("bad line");
    }

    this.hostTime = Number.parseInt(tokens[0]!, 10);
    this.deviceTime = Number.parseInt(tokens[1]!, 10);
    this.category = tokens[2]!.replace(/^\[ndnob\.|]$/g, "");
    this.value = tokens.slice(3).join(" ");
  }

  public readonly hostTime: number;
  public readonly deviceTime: number;
  public readonly category: string;
  public readonly value: string;

  public get int(): number {
    return Number.parseInt(this.value, 10);
  }
}

interface Events {
  error: Error;
  line: string;
  state: AppState;
}

/**
 * Control the device via device_conn.py bridge script.
 */
export class Device extends Emittery<Events> {
  private readonly child: ExecaChildProcess<Buffer>;

  constructor(port: string) {
    super();
    this.child = execa.command(`pipenv run python -u device_conn.py --port ${port}`, {
      buffer: false,
      encoding: null,
      stdin: "ignore",
      stdout: "pipe",
      stderr: "inherit",
    });
    // eslint-disable-next-line @typescript-eslint/no-floating-promises
    this.child.on("error", (err) => {
      void this.emit("error", err);
    });
    void this.handleStdout();
  }

  public readonly logs: string[] = [];
  public program: string[] = [];
  public states: AppState[] = [];

  public password = "";
  public wifiBssid = "";
  public bleMac = "";
  public bleMtu = 0;
  public certName = "";

  public heapTotal = 0;
  public heapFreeInitial = 0;
  public heapFreeFinal = 0;
  public heapFreeState: Partial<Record<AppState, number>> = {};

  /** Device application state. */
  public get state(): AppState {
    return this.states[this.states.length - 1] ?? AppState.Idle;
  }

  private async handleStdout() {
    for await (const line of byline(this.child.stdout!, { encoding: "utf-8" }) as AsyncIterable<string>) {
      let l: DeviceLogLine;
      try {
        l = new DeviceLogLine(line);
      } catch {
        continue;
      }
      this.logs.push(line);
      void this.emit("line", line);

      switch (l.category) {
        case "S.app": {
          const state = l.int as AppState;
          this.states.push(state);
          void this.emit("state", state);
          break;
        }
        case "O.program": {
          this.program = l.value.split(" ");
          break;
        }
        case "O.password":
          this.password = l.value;
          break;
        case "O.WiFi-BSSID":
          this.wifiBssid = l.value;
          break;
        case "O.BLE-MAC":
          this.bleMac = l.value;
          break;
        case "O.BLE-MTU":
          this.bleMtu = l.int;
          break;
        case "H.total":
          this.heapTotal = l.int;
          break;
        case "H.free-initial":
          this.heapFreeInitial = l.int;
          break;
        case "H.free-final":
          this.heapFreeFinal = l.int;
          break;
        case "H.free-prev-state": {
          if (this.states.length >= 2) {
            this.heapFreeState[this.states[this.states.length - 2]!] = l.int;
          }
          break;
        }
      }
    }
  }

  /** Close connection to the device. */
  public close(): void {
    this.child.kill();
  }

  public get result(): Device.Result {
    return {
      logs: this.logs,
      success: this.states.includes(AppState.Success),
      heapTotal: this.heapTotal,
      heapFreeInitial: this.heapFreeInitial,
      heapFreeFinal: this.heapFreeFinal,
      heapFreeState: this.heapFreeState,
    };
  }
}

export namespace Device {
  export interface Result {
    logs: string[];
    success: boolean;
    heapTotal: number;
    heapFreeInitial: number;
    heapFreeFinal: number;
    heapFreeState: Partial<Record<AppState, number>>;
  }
}
