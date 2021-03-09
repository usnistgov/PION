import { Authenticator } from "./authenticator";
import { AppState, Device } from "./device";
import { env } from "./env";
import { WifiClient } from "./wifi-client";

const d = new Device(env.deviceSerial);
d.on("error", console.error);
d.on("line", console.log);
d.on("state", (state) => {
  switch (state) {
    case AppState.WaitDirectConnect:
      void connectAndRunAuthenticator();
      break;
    case AppState.Final:
      d.close();
      break;
  }
});

async function connectAndRunAuthenticator() {
  let w: WifiClient|undefined;
  let a: Authenticator|undefined;
  try {
    await new Promise((resolve) => setTimeout(resolve, 1000));
    w = new WifiClient();
    await w.connect({
      ctrl: env.directWifiWpaCtrl,
      netif: env.directWifiNetif,
      ssid: env.directWifiSsid,
      passphrase: env.directWifiPassphrase,
      localIp: `${env.directWifiAuthIp}/${env.directWifiSubnet}`,
    });

    a = new Authenticator({
      deviceIp: env.directWifiDeviceIp,
      devicePort: 6363,
      mtu: undefined,
      keychain: env.keychain,
      caProfile: env.caProfile,
      deviceName: `${env.networkPrefix}/d${Date.now()}`,
      networkCredential: `${env.infraWifiSsid}\n${env.infraWifiPassphrase}\n${env.infraWifiGatewayIp}`,
      pakePassword: d.password,
    });
    a.on("line", console.log);
    await new Promise<void>((resolve, reject) => {
      void a!.once("error").then(reject);
      void a!.once("finish").then(resolve);
    });
  } finally {
    a?.close();
    await w?.disconnect();
  }
}
