import { setTimeout as delay } from "node:timers/promises";

import { execa } from "execa";

import type { DirectConnection } from "./direct-connection";

async function waitLinkUp(netif: string): Promise<void> {
  for (let i = 0; i < 100; ++i) {
    const result = await execa("ip", ["-j", "link", "show", netif]);
    const j = JSON.parse(result.stdout);
    if (j?.[0]?.operstate === "UP") {
      return;
    }
    await delay(100);
  }
  throw new Error("waitLinkUp timeout");
}

async function disablePowerSave(netif: string): Promise<void> {
  if (disablePowerSave.once.has(netif)) {
    return;
  }
  await execa("sudo", ["iw", "dev", netif, "set", "power_save", "off"], { stderr: "inherit" });
  disablePowerSave.once.add(netif);
}
disablePowerSave.once = new Set<string>();

async function setLocalIp(netif: string, localIp: string): Promise<void> {
  if (setLocalIp.once.get(netif) === localIp) {
    return;
  }
  await execa("sudo", ["ip", "addr", "replace", localIp, "dev", netif], { stderr: "inherit" });
  setLocalIp.once.set(netif, localIp);
}
setLocalIp.once = new Map<string, string>();

/** Control a WiFi station interface. */
export class WifiStation implements DirectConnection {
  private readonly wpaCliArgs: string[];
  private readonly netif: string;
  private readonly ssid: string;
  private readonly passphrase: string;
  private readonly localIp: string;
  private wpaNetwork = "";

  constructor({ ctrl, netif, ssid, passphrase, localIp }: WifiClient.Options) {
    this.wpaCliArgs = [`-p${ctrl}`, `-i${netif}`];
    this.netif = netif;
    this.ssid = ssid;
    this.passphrase = passphrase;
    this.localIp = localIp;
  }

  private async wpaCli(...args: string[]): Promise<string> {
    const result = await execa("wpa_cli", this.wpaCliArgs.concat(args));
    return result.stdout;
  }

  public async connect() {
    await disablePowerSave(this.netif);
    await setLocalIp(this.netif, this.localIp);

    this.wpaNetwork = await this.wpaCli("add_network");
    await this.wpaCli("set_network", this.wpaNetwork, "ssid", `"${this.ssid}"`);
    await this.wpaCli("set_network", this.wpaNetwork, "psk", `"${this.passphrase}"`);
    await this.wpaCli("select_network", this.wpaNetwork);
    await waitLinkUp(this.netif);
  }

  public async disconnect() {
    try {
      await this.wpaCli("remove_network", this.wpaNetwork);
    } catch {}
  }
}

export namespace WifiClient {
  export interface Options {
    ctrl: string;
    netif: string;
    ssid: string;
    passphrase: string;
    localIp: string;
  }
}
