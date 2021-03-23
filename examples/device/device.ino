#include "app.hpp"
#include <esp_wifi.h>

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  NDNPH_LOG_LINE("ndnob.O.program", "%s %s",
#if defined(NDNOB_DIRECT_WIFI)
                 "direct-wifi",
#elif defined(NDNOB_DIRECT_BLE)
                 "direct-ble",
#endif
#if defined(NDNOB_INFRA_UDP)
                 "infra-udp"
#elif defined(NDNOB_INFRA_ETHER)
                 "infra-ether"
#endif
  );
  NDNPH_LOG_LINE("ndnob.H.total", "%u", ESP.getHeapSize());
  NDNPH_LOG_LINE("ndnob.H.free-initial", "%u", ESP.getFreeHeap());

  esp8266ndn::setLogOutput(Serial);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  {
    uint8_t mac[6];
    ndnph::port::RandomSource::generate(mac, sizeof(mac));
    mac[0] &= ~0x01;
    mac[0] |= 0x02;
    esp_wifi_set_mac(WIFI_IF_STA, mac);
  }
  WiFi.disconnect();
  delay(100);
}

void
loop()
{
  ndnob_device_app::loop();
}
