/**
 * Packet direction.
 *
 * "<" indicates a packet from the device.
 * ">" indicates a packet to the device.
 */
export type PacketDir = "<" | ">";

/** Packet direction and length information. */
export interface PacketMeta {
  ts: number;
  deviceTime?: number;
  dir: PacketDir;
  len: number;
  protocolStep?: string;
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

/** Recognize packets for protocol steps. */
export function labelPacketSteps(seq: ProtocolSequence, pkts: PacketMeta[]): void {
  let i = 0;
  for (const [dir, step] of seq) {
    let pkt = pkts[i];
    if (pkt?.dir !== dir) {
      throw new Error(`missing packet for step ${step}`);
    }

    do {
      pkt!.protocolStep = step;
      ++i;
    } while ((pkt = pkts[i])?.dir === dir);
  }

  if (i !== pkts.length) {
    throw new Error(`unexpected packet after ${i}`);
  }
}
