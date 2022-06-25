#ifndef PION_COMMON_HPP
#define PION_COMMON_HPP

#ifdef ARDUINO_ARCH_ESP32
#include <sdkconfig.h>
#ifdef CONFIG_BT_ENABLED
#include <NimBLEDevice.h>
#endif
#include <esp8266ndn.h>
#else
#include <NDNph.h>
#endif

#endif // PION_COMMON_HPP
