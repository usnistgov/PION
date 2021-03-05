#include "app.hpp"
#include <esp_wifi.h>

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  NDNOB_LOG_MSG("H.total", "%u\n", ESP.getHeapSize());
  NDNOB_LOG_MSG("H.free-initial", "%u\n", ESP.getFreeHeap());

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
