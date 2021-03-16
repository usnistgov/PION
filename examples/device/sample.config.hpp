#ifndef NDNOB_DEVICE_CONFIG_HPP
#define NDNOB_DEVICE_CONFIG_HPP

// select direct transport
#define NDNOB_DIRECT_WIFI
// #define NDNOB_DIRECT_BLE

// hard-coded direct transport credentials
#define NDNOB_DIRECT_AP_SSID "ndnob-direct"
#define NDNOB_DIRECT_AP_PASS "ndnob-direct-password"
#define NDNOB_DIRECT_BLE_NAME "ndnob-direct"

// select infrastructure transport
#define NDNOB_INFRA_UDP
// #define NDNOB_INFRA_ETHER

// skip steps, for code size measurement
// #define NDNOB_SKIP_PAKE
// #define NDNOB_SKIP_NDNCERT

// infrastructure network credentials, used when NDNOB_SKIP_PAKE
#define NDNOB_INFRA_NC "ndnob\nndnob-infra-password\n192.168.36.1"

#endif // NDNOB_DEVICE_CONFIG_HPP
