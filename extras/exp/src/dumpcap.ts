import { execa, type ExecaChildProcess } from "execa";

import type { PacketDir, PacketMeta } from "./packet";

/** Capture packets on a network interface. */
export class Dumpcap {
  private readonly child: ExecaChildProcess<Buffer>;

  /** PCAP trace, available after `close()`. */
  public pcap?: Buffer;

  /** Start packet capture. */
  constructor(netif: string, filter?: string) {
    this.child = execa("dumpcap", [
      "-i", netif, // capture interface
      "-p", // no promiscuous mode
      "-P", // save as pcap instead of pcapng
      "-q", // don't display continuous packet count
      "-w", "-", // output to stdout
      ...(filter ? ["-f", filter] : []), // filter
    ], {
      encoding: null,
      stdin: "ignore",
      stdout: "pipe",
      stderr: "inherit",
    });
  }

  /** Stop packet capture. */
  public async close(): Promise<void> {
    this.child.kill();
    const result = await this.child;
    this.pcap = result.stdout;
  }

  /** Extract PacketMeta from `this.pcap` with pion-parsepcap program. */
  public async extract(arg: string): Promise<PacketMeta[]> {
    if (!this.pcap) {
      return [];
    }

    const result = await execa("pion-pcapparse", [arg], {
      encoding: "utf8",
      input: this.pcap,
      stdout: "pipe",
      stderr: "inherit",
    });

    const packets: PacketMeta[] = [];
    for (const line of result.stdout.split("\n")) {
      const m = /^(\d+) ([<>]) (\d+)$/.exec(line);
      if (!m) {
        throw new Error(`bad line: ${line}`);
      }
      packets.push({
        ts: Number.parseInt(m[1]!, 10),
        dir: m[2] as PacketDir,
        len: Number.parseInt(m[3]!, 10),
      });
    }
    return packets;
  }
}
