#include "app.hpp"
#include <esp_wifi.h>

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  esp8266ndn::setLogOutput(Serial);

  WiFi.persistent(false);
  {
    uint8_t mac[6];
    ndnph::port::RandomSource::generate(mac, sizeof(mac));
    mac[0] &= ~0x01;
    mac[0] |= 0x02;
    esp_wifi_set_mac(ESP_IF_WIFI_STA, mac);
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void
loop()
{
  ndnob_device_app::loop();
}
