import execa, { ExecaChildProcess } from "execa";

/** Capture packets on a network interface. */
export class Dumpcap {
  private readonly child: ExecaChildProcess<Buffer>;

  /** PCAP trace, available after `stop()`. */
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
}
