#include <ndn-onboarding.h>

#include <Arduino.h>
#include <ArduinoUnit.h>

void
setup()
{
  Serial.begin(115200);
}

void
loop()
{
  Test::run();
}
