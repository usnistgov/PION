#include "app.hpp"

namespace ndnob_device_app {

#if defined(NDNOB_DIRECT_WIFI)
static std::unique_ptr<esp8266ndn::UdpTransport> transport;
#elif defined(NDNOB_DIRECT_BLE)
static std::unique_ptr<esp8266ndn::BleServerTransport> transport;
#else
#error "need either NDNOB_DIRECT_WIFI or NDNOB_DIRECT_BLE"
#endif
static std::unique_ptr<ndnph::Face> face;
static std::unique_ptr<ndnob::pake::Device> device;

void
doDirectConnect()
{
  GotoState gotoState;

#if defined(NDNOB_DIRECT_WIFI)
  WiFi.softAP(NDNOB_DIRECT_AP_SSID, NDNOB_DIRECT_AP_PASS, 1, 0, 1);
  while (WiFi.softAPgetStationNum() == 0) {
    delay(100);
  }

  transport.reset(new esp8266ndn::UdpTransport);
  if (!transport->beginListen(6363, WiFi.softAPIP())) {
    NDNOB_LOG_ERR("transport.beginListen error");
    return;
  }

  face.reset(new ndnph::Face(*transport));

  gotoState(State::WaitPake);
#endif
}

static bool
initPake()
{
  device.reset(new ndnob::pake::Device(ndnob::pake::Device::Options{
    face : *face,
  }));
  if (!device->begin(getPassword())) {
    NDNOB_LOG_ERR("device.begin error");
    return false;
  }
  return true;
}

void
waitPake()
{
  face->loop();

  if (device == nullptr && !initPake()) {
    state = State::Failure;
    return;
  }

  auto deviceState = device->getState();
  NDNOB_LOG_STATE("pake-device", deviceState);
  switch (deviceState) {
    case ndnob::pake::Device::State::Success: {
      state = State::WaitDirectDisconnect;
      break;
    }
    case ndnob::pake::Device::State::Failure: {
      NDNOB_LOG_ERR("pake-device failure");
      state = State::Failure;
      break;
    }
    default: {
      break;
    }
  }
}

void
doDirectDisconnect()
{
  for (int i = 0; i < 20; ++i) {
    face->loop();
    delay(100);
  }

#if defined(NDNOB_DIRECT_WIFI)
  WiFi.softAPdisconnect(true);
  delay(5000);
  state = State::WaitInfraConnect;
#else
  state = State::Failure;
#endif
}

const ndnob::pake::Device*
getPakeDevice()
{
  return device.get();
}

void
deletePakeDevice()
{
  device.reset();
  face.reset();
  transport.reset();
}

} // namespace ndnob_device_app
