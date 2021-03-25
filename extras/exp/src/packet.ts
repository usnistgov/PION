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

export function pairTxRx(dump: readonly PacketMeta[], device: readonly PacketMeta[]): PacketTxRx[] {
  const packets: PacketTxRx[] = [];
  for (let i = 0, j = 0; i < dump.length && j < device.length; ++i, ++j) {
    const x = dump[i]!;
    const y = device[j]!;
    if (x.dir === y.dir && x.len === y.len) {
      switch (x.dir) {
        case "<":
          packets.push({ dir: "<", len: x.len, txTime: y.ts, rxTime: x.ts });
          break;
        case ">":
          packets.push({ dir: ">", len: x.len, txTime: x.ts, rxTime: y.ts });
          break;
      }
    }
  }
  return packets;
}

export type ProtocolSequence = ReadonlyArray<[PacketDir, string, boolean?]>;

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
    ["<", "credential-response", true],
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

export interface ProtocolPacket {
  id: string;
  dir: PacketDir;
  fragments: PacketTxRx[];
}

export function labelExchanges(seq: ProtocolSequence, txrx: readonly PacketTxRx[]): ProtocolPacket[] {
  const exchanges: ProtocolPacket[] = [];
  let i = 0;
  for (const [dir, id, optional = false] of seq) {
    let pkt = txrx[i];
    if (!pkt || pkt.dir !== dir) {
      if (optional) {
        exchanges.push({ id, dir, fragments: [] });
      } else {
        throw new Error(`missing packet for step ${dir} ${id}`);
      }
      continue;
    }

    const fragments: PacketTxRx[] = [];
    do {
      fragments.push(pkt!);
      ++i;
    } while ((pkt = txrx[i])?.dir === dir);
    exchanges.push({ id, dir, fragments });
  }
  return exchanges;
}
