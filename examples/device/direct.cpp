#include "app.hpp"

namespace pion_device_app {

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

#if defined(PION_DIRECT_WIFI)
static std::unique_ptr<esp8266ndn::UdpTransport> transport;
#elif defined(PION_DIRECT_BLE)
static std::unique_ptr<esp8266ndn::BleServerTransport> transport;
static std::unique_ptr<FragReass> fragReass;
#else
#error "need either PION_DIRECT_WIFI or PION_DIRECT_BLE"
#endif
static std::unique_ptr<ndnph::transport::Tracer> transportTracer;
static std::unique_ptr<ndnph::Face> face;
static std::unique_ptr<pion::pake::Device> device;

void
doDirectConnect()
{
  GotoState gotoState;

#if defined(PION_DIRECT_WIFI)
  WiFi.softAP(PION_DIRECT_AP_SSID, PION_DIRECT_AP_PASS, 1, 0, 1);
  NDNPH_LOG_MSG("pion.O.WiFi-BSSID", "");
  Serial.println(WiFi.softAPmacAddress());

  while (WiFi.softAPgetStationNum() == 0) {
    delay(100);
  }

  transport.reset(new esp8266ndn::UdpTransport);
  if (!transport->beginListen(6363, WiFi.softAPIP())) {
    PION_LOG_ERR("transport.beginListen error");
    return;
  }
#elif defined(PION_DIRECT_BLE)
  transport.reset(new esp8266ndn::BleServerTransport);
  if (!transport->begin(PION_DIRECT_BLE_NAME)) {
    PION_LOG_ERR("transport.begin error");
    return;
  }

  NDNPH_LOG_MSG("pion.O.BLE-MAC", "");
  Serial.println(transport->getAddr());
  NDNPH_LOG_LINE("pion.O.BLE-MTU", "%d\n", static_cast<int>(transport->getMtu()));

  while (!transport->isUp()) {
    delay(100);
  }
#endif

  transportTracer.reset(new ndnph::transport::Tracer(*transport, "pion.T.direct"));
  face.reset(new ndnph::Face(*transportTracer));
#if defined(PION_DIRECT_BLE)
  fragReass.reset(new FragReass(*face, transport->getMtu()));
#endif
  gotoState(State::WaitPake);
}

#ifndef PION_SKIP_PAKE
static bool
initPake()
{
  device.reset(new pion::pake::Device(pion::pake::Device::Options{
    face : *face,
  }));
  if (!device->begin(getPassword())) {
    PION_LOG_ERR("device.begin error");
    return false;
  }
  return true;
}
#endif // PION_SKIP_PAKE

void
waitPake()
{
  face->loop();

#ifndef PION_SKIP_PAKE
  if (device == nullptr && !initPake()) {
    state = State::Failure;
    return;
  }

  auto deviceState = device->getState();
  PION_LOG_STATE("pake-device", deviceState);
  switch (deviceState) {
    case pion::pake::Device::State::Success: {
      state = State::WaitDirectDisconnect;
      break;
    }
    case pion::pake::Device::State::Failure: {
      PION_LOG_ERR("pake-device failure");
      state = State::Failure;
      break;
    }
    default: {
      break;
    }
  }
#else
  state = State::WaitDirectDisconnect;
#endif // PION_SKIP_PAKE
}

void
doDirectDisconnect()
{
  for (int i = 0; i < 20; ++i) {
    face->loop();
    delay(100);
  }
#ifndef PION_SKIP_PAKE
  face->removeHandler(*device);
#endif
  face.reset();

#if defined(PION_DIRECT_WIFI)
  WiFi.softAPdisconnect(true);
  delay(4000);
#elif defined(PION_DIRECT_BLE)
  ::BLEDevice::deinit(true);
  fragReass.reset();
#endif

  transportTracer.reset();
  transport.reset();
  state = State::WaitInfraConnect;
}

const pion::pake::Device*
getPakeDevice()
{
  return device.get();
}

void
deletePakeDevice()
{
  device.reset();
}

} // namespace pion_device_app
