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

static bool
initDirectConnect()
{
#if defined(NDNOB_DIRECT_WIFI)
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(NDNOB_DIRECT_AP_SSID, NDNOB_DIRECT_AP_PASS, 1, 0, 1);

  transport.reset(new esp8266ndn::UdpTransport);
  if (!transport->beginListen(6363, WiFi.softAPIP())) {
    NDNOB_LOG_ERR("transport.beginListen error");
    return false;
  }

  face.reset(new ndnph::Face(*transport));
  return true;
#else
  return false;
#endif
}

void
waitDirectConnect()
{
  if (face == nullptr && !initDirectConnect()) {
    state = State::Failure;
    return;
  }

#if defined(NDNOB_DIRECT_WIFI)
  if (WiFi.softAPgetStationNum() > 0) {
    state = State::WaitPake;
  }
#endif
}

bool
initPake()
{
  device.reset(new ndnob::pake::Device(ndnob::pake::Device::Options{
    face : *face,
  }));
  if (!device->begin(ndnph::tlv::Value::fromString("password"))) {
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
waitDirectDisconnect()
{
  static int i = 0;
  if (++i < 50) {
    face->loop();
    return;
  }

  device.reset();
  face.reset();
  transport.reset();
#if defined(NDNOB_DIRECT_WIFI)
  WiFi.softAPdisconnect(true);
  state = State::WaitInfraConnect;
#else
  state = State::Failure;
#endif
}

} // namespace ndnob_device_app
