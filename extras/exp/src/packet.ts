/**
 * Packet direction.
 *
 * "<" indicates a packet from the device.
 * ">" indicates a packet to the device.
 */
export type PacketDir = "<"|">";

/** Packet direction and length information. */
export interface PacketMeta {
  ts: number;
  deviceTime?: number;
  dir: PacketDir;
  len: number;
}

/** Paired packet TX and RX. */
export interface PacketTxRx {
  dir: PacketDir;
  len: number;
  txTime: number;
  rxTime: number;
}

/**
 * Pair packet TX and RX based on direction and length.
 * @param dump packets found in dumpcap.
 * @param device packets reported by device.
 */
export function* pairTxRx(dump: readonly PacketMeta[], device: readonly PacketMeta[]): Iterable<PacketTxRx> {
  if (dump.length !== device.length) {
    throw new Error(`unmatched packet counts dump=${dump.length} device=${device.length}`);
  }

  for (let i = 0; i < dump.length && i < device.length; ++i) {
    const x = dump[i]!;
    const y = device[i]!;
    if (x.dir !== y.dir || x.len !== y.len) {
      throw new Error(`unmatched packets at ${i}`);
    }

    switch (x.dir) {
      case "<":
        yield { dir: "<", len: x.len, txTime: y.ts, rxTime: x.ts };
        break;
      case ">":
        yield { dir: ">", len: x.len, txTime: x.ts, rxTime: y.ts };
        break;
    }
  }
}

/** Definition of protocol sequence. */
export type ProtocolSequence = ReadonlyArray<[PacketDir, string]>;

export namespace ProtocolSequence {
  export const direct: ProtocolSequence = [
    [">", "pake-request"],
    ["<", "pake-response"],
    [">", "confirm-request"],
    ["<", "ca-profile-interest"],
    [">", "ca-profile-data"],
    ["<", "authenticator-cert-interest"],
    [">", "authenticator-cert-data"],
    ["<", "confirm-response"],
    [">", "credential-request"],
    ["<", "temp-cert-interest"],
    [">", "temp-cert-data"],
    ["<", "credential-response"],
  ];

  export const infra: ProtocolSequence = [
    ["<", "NEW-request"],
    [">", "NEW-response"],
    ["<", "CHALLENGE-start-request"],
    [">", "CHALLENGE-start-response"],
    ["<", "CHALLENGE-finish-request"],
    [">", "CHALLENGE-finish-response"],
    ["<", "device-cert-interest"],
    [">", "device-cert-data"],
  ];
}

/** Recognized packets for a protocol step. */
export interface PacketStep {
  id: string;
  dir: PacketDir;
  fragments: PacketTxRx[];
}

/** Recognize packets for protocol steps. */
export function* labelPacketSteps(seq: ProtocolSequence, txrx: readonly PacketTxRx[]): Iterable<PacketStep> {
  let i = 0;
  for (const [dir, id] of seq) {
    let pkt = txrx[i];
    if (!pkt || pkt.dir !== dir) {
      throw new Error(`missing packet for step ${dir} ${id}`);
    }

    const fragments: PacketTxRx[] = [];
    do {
      fragments.push(pkt!);
      ++i;
    } while ((pkt = txrx[i])?.dir === dir);
    yield { id, dir, fragments };
  }

  if (i !== txrx.length) {
    throw new Error(`unexpected packet after ${i}`);
  }
}
