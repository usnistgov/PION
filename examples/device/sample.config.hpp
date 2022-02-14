#ifndef PION_DEVICE_CONFIG_HPP
#define PION_DEVICE_CONFIG_HPP

// select direct transport
#define PION_DIRECT_WIFI
// #define PION_DIRECT_BLE

// hard-coded direct transport credentials
#define PION_DIRECT_AP_SSID "pion-direct"
#define PION_DIRECT_AP_PASS "pion-direct-password"
#define PION_DIRECT_BLE_NAME "pion-direct"

// select infrastructure transport
#define PION_INFRA_UDP
// #define PION_INFRA_ETHER

// skip steps, for code size measurement
// #define PION_SKIP_PAKE
// #define PION_SKIP_NDNCERT

// infrastructure network credentials, used when PION_SKIP_PAKE
#define PION_INFRA_NC "pion\npion-infra-password\n192.168.36.1"

// ping server name, used when PION_SKIP_NDNCERT
#define PION_PING_NAME "/pion/device"

#endif // PION_DEVICE_CONFIG_HPP
