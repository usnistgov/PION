#include "app.hpp"

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  esp8266ndn::setLogOutput(Serial);
}

void
loop()
{
  ndnob_device_app::loop();
}
