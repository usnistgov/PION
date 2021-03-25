import pDefer, { DeferredPromise } from "p-defer";

import { Authenticator } from "./authenticator";
import { BleBridge } from "./ble-bridge";
import { AppState, Device } from "./device";
import type { DirectConnection } from "./direct-connection";
import { Dumpcap } from "./dumpcap";
import { env, INFRA_WIFI_NETIF_MACADDR } from "./env";
import { labelPacketSteps, PacketMeta, PacketStep, PacketTxRx, pairTxRx, ProtocolSequence } from "./packet";
import { WifiStation } from "./wifi-station";

function delay(duration: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, duration));
}

async function analyzeDump(
    devicePackets: PacketMeta[],
    dump: Dumpcap|undefined, extractArg: string,
    seq: ProtocolSequence): Promise<Run.DumpResult> {
  const result: Run.DumpResult = {
    devicePackets,
  };
  if (!dump?.pcap) {
    return result;
  }
  result.dumpPcap = dump.pcap.toString("base64");
  try {
    result.dumpPackets = await dump.extract(extractArg);
    result.txrx = Array.from(pairTxRx(result.dumpPackets, devicePackets));
    result.exchanges = Array.from(labelPacketSteps(seq, result.txrx));
  } catch (err: unknown) {
    result.error = `${err}`;
  }
  return result;
}

/** Perform a single run of the experiment. */
export class Run {
  private l!: Console;
  private defer!: DeferredPromise<Run.Result>;
  private timeout!: NodeJS.Timeout;
  private device?: Device;
  private directDump?: Dumpcap;
  private directDumpExtractArg = "";
  private directConn?: DirectConnection;
  private authenticatorConn?: Pick<Authenticator.Options, "deviceIp"|"devicePort"|"mtu">;
  private authenticator?: Authenticator;
  private infraDump?: Dumpcap;

  public run({
    logger = process.stderr,
  }: Run.Options = {}): Promise<Run.Result> {
    this.l = new console.Console(logger);
    this.defer = pDefer<Run.Result>();
    this.timeout = setTimeout(() => {
      void this.fail("timeout");
    }, 120000);
    this.initDevice();
    return this.defer.promise;
  }

  private initDevice(): void {
    this.device = new Device(env.deviceSerial);
    this.device.on("state", this.handleDeviceState);
    this.device.on("line", (line) => this.l.debug("device", line));
    // eslint-disable-next-line promise/prefer-await-to-then
    void this.device.once("error").then((err) => this.fail(err));
  }

  private handleDeviceState = async (state: AppState) => {
    switch (state) {
      case AppState.WaitDirectConnect:
        await this.directConnect();
        break;
      case AppState.WaitPake:
        this.startAuthenticator();
        break;
      case AppState.WaitDirectDisconnect:
        await delay(500);
        await this.directDisconnect();
        break;
      case AppState.WaitInfraConnect:
        this.startInfraDump();
        break;
      case AppState.Final:
        await this.finish();
        break;
    }
  };

  private async directConnect(): Promise<void> {
    await delay(1000);
    if (this.device!.program.includes("direct-wifi")) {
      this.directDumpExtractArg = "--ap";
      this.directDump = new Dumpcap(env.directWifiNetif);
      await delay(500);

      this.directConn = new WifiStation({
        ctrl: env.directWifiWpaCtrl,
        netif: env.directWifiNetif,
        ssid: env.directWifiSsid,
        passphrase: env.directWifiPassphrase,
        localIp: `${env.directWifiAuthIp}/${env.directWifiSubnet}`,
      });

      this.authenticatorConn = {
        deviceIp: env.directWifiDeviceIp,
        devicePort: 6363,
        mtu: undefined,
      };
    } else if (this.device!.program.includes("direct-ble")) {
      this.directDumpExtractArg = "--ble";
      this.directDump = new Dumpcap("lo", `host ${BleBridge.IP}`);
      await delay(500);

      this.directConn = new BleBridge(this.device!.bleAddr, env.directBleBridgePath);

      this.authenticatorConn = {
        deviceIp: BleBridge.IP,
        devicePort: BleBridge.PORT,
        mtu: this.device!.bleMtu,
      };
    } else {
      throw new Error("unknown direct connection method");
    }

    await this.directConn.connect();
  }

  private startAuthenticator(): void {
    this.authenticator = new Authenticator({
      ...this.authenticatorConn!,
      keychain: env.keychain,
      caProfile: env.caProfile,
      deviceName: `${env.networkPrefix}/d${Date.now()}`,
      networkCredential: `${env.infraWifiSsid}\n${env.infraWifiPassphrase}\n${env.infraWifiGatewayIp}`,
      pakePassword: this.device!.password,
    });
    this.authenticator.on("line", (line) => this.l.debug("authenticator", line));
    // eslint-disable-next-line promise/prefer-await-to-then
    void this.authenticator.once("error").then((err) => this.fail(err));
  }

  private async directDisconnect(): Promise<void> {
    await this.directConn?.disconnect();
  }

  private startInfraDump(): void {
    this.infraDump = new Dumpcap(env.infraWifiNetif);
  }

  private cleanup = async () => {
    clearTimeout(this.timeout);
    this.device?.close();
    this.authenticator?.close();
    await this.directConn?.disconnect();
    await this.directDump?.close();
    await this.infraDump?.close();
  };

  private async fail(err: unknown): Promise<void> {
    await this.cleanup();
    this.defer.resolve({
      program: this.device?.program,
      device: this.device?.result,
      authenticator: this.authenticator?.result,
      error: `${err}`,
    });
  }

  private async finish(): Promise<void> {
    await delay(500);
    await this.cleanup();

    const direct = await analyzeDump(this.device!.directPackets,
      this.directDump, this.directDumpExtractArg, ProtocolSequence.direct);
    const infra = await analyzeDump(this.device!.infraPackets,
      this.infraDump, `--sta=${INFRA_WIFI_NETIF_MACADDR}`, ProtocolSequence.infra);
    this.defer.resolve({
      program: this.device!.program,
      device: this.device!.result,
      authenticator: this.authenticator?.result,
      direct,
      infra,
    });
  }
}

export namespace Run {
  export interface Options {
    logger?: NodeJS.WritableStream;
  }

  export interface Result {
    program?: string[];
    device?: Device.Result;
    authenticator?: Authenticator.Result;
    direct?: DumpResult;
    infra?: DumpResult;
    error?: string;
  }

  export interface DumpResult {
    devicePackets?: PacketMeta[];
    dumpPcap?: string;
    dumpPackets?: PacketMeta[];
    txrx?: PacketTxRx[];
    exchanges?: PacketStep[];
    error?: string;
  }
}
