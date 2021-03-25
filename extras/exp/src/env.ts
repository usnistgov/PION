import { makeEnv, parsers } from "@strattadb/environment";
import dotenv from "dotenv";
import * as os from "os";

dotenv.config();

export const env = makeEnv({
  deviceSerial: {
    envVarName: "NDNOB_DEVICE_SERIAL",
    parser: parsers.string,
    required: true,
  },
  caProfile: {
    envVarName: "NDNOB_CA_PROFILE",
    parser: parsers.string,
    required: true,
  },
  keychain: {
    envVarName: "NDNOB_AUTH_KEYCHAIN",
    parser: parsers.string,
    required: true,
  },
  networkPrefix: {
    envVarName: "NDNOB_NETWORK_PREFIX",
    parser: parsers.string,
    required: true,
  },
  directWifiWpaCtrl: {
    envVarName: "NDNOB_DIRECT_WIFI_WPA_CTRL",
    parser: parsers.string,
    required: true,
  },
  directWifiNetif: {
    envVarName: "NDNOB_DIRECT_WIFI_NETIF",
    parser: parsers.string,
    required: true,
  },
  directWifiSsid: {
    envVarName: "NDNOB_DIRECT_WIFI_SSID",
    parser: parsers.string,
    required: true,
  },
  directWifiPassphrase: {
    envVarName: "NDNOB_DIRECT_WIFI_PASS",
    parser: parsers.string,
    required: true,
  },
  directWifiDeviceIp: {
    envVarName: "NDNOB_DIRECT_WIFI_DEVICE_IP",
    parser: parsers.ipAddress,
    required: true,
  },
  directWifiAuthIp: {
    envVarName: "NDNOB_DIRECT_WIFI_AUTH_IP",
    parser: parsers.ipAddress,
    required: true,
  },
  directWifiSubnet: {
    envVarName: "NDNOB_DIRECT_WIFI_SUBNET",
    parser: parsers.positiveInteger,
    required: true,
  },
  directBleBridgePath: {
    envVarName: "NDNOB_DIRECT_BLE_BRIDGE_PATH",
    parser: parsers.string,
    required: true,
  },
  infraWifiNetif: {
    envVarName: "NDNOB_INFRA_WIFI_NETIF",
    parser: parsers.string,
    required: true,
  },
  infraWifiSsid: {
    envVarName: "NDNOB_INFRA_WIFI_SSID",
    parser: parsers.string,
    required: true,
  },
  infraWifiPassphrase: {
    envVarName: "NDNOB_INFRA_WIFI_PASS",
    parser: parsers.string,
    required: true,
  },
  infraWifiGatewayIp: {
    envVarName: "NDNOB_INFRA_WIFI_GW_IP",
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
