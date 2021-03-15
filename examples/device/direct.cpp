#include "app.hpp"

namespace ndnob_device_app {

class FragReass
{
public:
  explicit FragReass(ndnph::Face& face, uint16_t mtu)
    : m_fragmenter(m_region, mtu)
    , m_reassembler(m_region)
  {
    face.setFragmenter(m_fragmenter);
    face.setReassembler(m_reassembler);
  }

private:
  ndnph::StaticRegion<1200> m_region;
  ndnph::lp::Fragmenter m_fragmenter;
  ndnph::lp::Reassembler m_reassembler;
};

#if defined(NDNOB_DIRECT_WIFI)
static std::unique_ptr<esp8266ndn::UdpTransport> transport;
#elif defined(NDNOB_DIRECT_BLE)
static std::unique_ptr<esp8266ndn::BleServerTransport> transport;
static std::unique_ptr<FragReass> fragReass;
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
  NDNOB_LOG_MSG("O.WiFi-BSSID", "");
  Serial.println(WiFi.softAPmacAddress());

  while (WiFi.softAPgetStationNum() == 0) {
    delay(100);
  }

  transport.reset(new esp8266ndn::UdpTransport);
  if (!transport->beginListen(6363, WiFi.softAPIP())) {
    NDNOB_LOG_ERR("transport.beginListen error");
    return;
  }
#elif defined(NDNOB_DIRECT_BLE)
  transport.reset(new esp8266ndn::BleServerTransport);
  if (!transport->begin(NDNOB_DIRECT_BLE_NAME)) {
    NDNOB_LOG_ERR("transport.begin error");
    return;
  }

  NDNOB_LOG_MSG("O.BLE-MAC", "");
  Serial.println(transport->getAddr());
  NDNOB_LOG_MSG("O.BLE-MTU", "%d\n", static_cast<int>(transport->getMtu()));

  while (!transport->isUp()) {
    delay(100);
  }
#endif

  face.reset(new ndnph::Face(*transport));
#if defined(NDNOB_DIRECT_BLE)
  fragReass.reset(new FragReass(*face, transport->getMtu()));
#endif
  gotoState(State::WaitPake);
}

#ifndef NDNOB_SKIP_PAKE
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
#endif // NDNOB_SKIP_PAKE

void
waitPake()
{
  face->loop();

#ifndef NDNOB_SKIP_PAKE
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
#else
  state = State::WaitDirectDisconnect;
#endif // NDNOB_SKIP_PAKE
}

void
doDirectDisconnect()
{
  for (int i = 0; i < 20; ++i) {
    face->loop();
    delay(100);
  }
#ifndef NDNOB_SKIP_PAKE
  face->removeHandler(*device);
#endif
  face.reset();

#if defined(NDNOB_DIRECT_WIFI)
  WiFi.softAPdisconnect(true);
  delay(4000);
#elif defined(NDNOB_DIRECT_BLE)
  ::BLEDevice::deinit(true);
  fragReass.reset();
#endif

  transport.reset();
  state = State::WaitInfraConnect;
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
}

} // namespace ndnob_device_app
