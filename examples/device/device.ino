#include "secrets.hpp"
#include <WiFi.h>
#include <ndn-onboarding.h>

esp8266ndn::UdpTransport transport;
ndnph::Face face(transport);

std::unique_ptr<ndnob::pake::Device> device;

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  esp8266ndn::setLogOutput(Serial);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  delay(1000);
  Serial.println(WiFi.localIP());

  if (!transport.beginListen(6363, WiFi.localIP())) {
    Serial.println("transport.beginListen error");
    ESP.restart();
  }

  device.reset(new ndnob::pake::Device(ndnob::pake::Device::Options{
    face : face,
  }));
  if (!device->begin(ndnph::tlv::Value::fromString("password"))) {
    Serial.println("device.begin error");
    ESP.restart();
  }
}

void
loop()
{
  face.loop();

  static auto lastState = ndnob::pake::Device::State::Idle;
  auto state = device->getState();
  if (state != lastState) {
    lastState = state;
    Serial.printf("state=%d\n", static_cast<int>(state));
    Serial.print("device-name=");
    Serial.println(device->getDeviceName());
  }
}
