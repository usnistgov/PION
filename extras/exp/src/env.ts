import * as os from "node:os";

import { makeEnv, parsers } from "@strattadb/environment";
import dotenv from "dotenv";

dotenv.config();

export const env = makeEnv({
  deviceSerial: {
    envVarName: "PION_DEVICE_SERIAL",
    parser: parsers.string,
    required: true,
  },
  caProfile: {
    envVarName: "PION_CA_PROFILE",
    parser: parsers.string,
    required: true,
  },
  keychain: {
    envVarName: "PION_AUTH_KEYCHAIN",
    parser: parsers.string,
    required: true,
  },
  networkPrefix: {
    envVarName: "PION_NETWORK_PREFIX",
    parser: parsers.string,
    required: true,
  },
  directWifiWpaCtrl: {
    envVarName: "PION_DIRECT_WIFI_WPA_CTRL",
    parser: parsers.string,
    required: true,
  },
  directWifiNetif: {
    envVarName: "PION_DIRECT_WIFI_NETIF",
    parser: parsers.string,
    required: true,
  },
  directWifiSsid: {
    envVarName: "PION_DIRECT_WIFI_SSID",
    parser: parsers.string,
    required: true,
  },
  directWifiPassphrase: {
    envVarName: "PION_DIRECT_WIFI_PASS",
    parser: parsers.string,
    required: true,
  },
  directWifiDeviceIp: {
    envVarName: "PION_DIRECT_WIFI_DEVICE_IP",
    parser: parsers.ipAddress,
    required: true,
  },
  directWifiAuthIp: {
    envVarName: "PION_DIRECT_WIFI_AUTH_IP",
    parser: parsers.ipAddress,
    required: true,
  },
  directWifiSubnet: {
    envVarName: "PION_DIRECT_WIFI_SUBNET",
    parser: parsers.positiveInteger,
    required: true,
  },
  directBleBridgePath: {
    envVarName: "PION_DIRECT_BLE_BRIDGE_PATH",
    parser: parsers.string,
    required: true,
  },
  infraWifiNetif: {
    envVarName: "PION_INFRA_WIFI_NETIF",
    parser: parsers.string,
    required: true,
  },
  infraWifiSsid: {
    envVarName: "PION_INFRA_WIFI_SSID",
    parser: parsers.string,
    required: true,
  },
  infraWifiPassphrase: {
    envVarName: "PION_INFRA_WIFI_PASS",
    parser: parsers.string,
    required: true,
  },
  infraWifiGatewayIp: {
    envVarName: "PION_INFRA_WIFI_GW_IP",
    parser: parsers.ipAddress,
    required: true,
  },
});

export const INFRA_WIFI_NETIF_MACADDR = (() => {
  const netif = os.networkInterfaces()[env.infraWifiNetif];
  if (!netif) {
    throw new Error(`network interface ${env.infraWifiNetif} is absent`);
  }
  return netif[0]!.mac;
})();
