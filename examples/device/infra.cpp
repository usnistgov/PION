#include "app.hpp"

namespace ndnob_device_app {

#if defined(NDNOB_INFRA_UDP)
static std::unique_ptr<esp8266ndn::UdpTransport> transport;
#elif defined(NDNOB_INFRA_ETHER)
static std::unique_ptr<esp8266ndn::EthernetTransport> transport;
#else
#error "need either NDNOB_INFRA_UDP or NDNOB_INFRA_ETHER"
#endif
static std::unique_ptr<ndnph::Face> face;
static ndnph::StaticRegion<2048> oRegion;
static ndnph::EcPrivateKey pvt;
static ndnph::EcPublicKey pub;
static std::unique_ptr<ndnph::ndncert::client::PossessionChallenge> challenge;
static ndnph::Data oCert;

static bool
waitWiFiConnect()
{
  while (true) {
    auto st = WiFi.status();
    NDNOB_LOG_STATE("WiFi", st);
    switch (st) {
      case WL_CONNECTED:
        return true;
      case WL_NO_SSID_AVAIL:
      case WL_CONNECT_FAILED:
      case WL_CONNECTION_LOST:
        return false;
      default:
        break;
    }
    delay(100);
  }
}

void
doInfraConnect()
{
  GotoState gotoState;

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(NDNOB_INFRA_STA_SSID, NDNOB_INFRA_STA_PASS);
  if (!waitWiFiConnect()) {
    NDNOB_LOG_ERR("WiFi.begin failure");
    return;
  }

#if defined(NDNOB_INFRA_UDP)
  transport.reset(new esp8266ndn::UdpTransport);
  IPAddress remote;
  if (!remote.fromString(NDNOB_INFRA_UDP_REMOTE)) {
    NDNOB_LOG_ERR("IPAddress.fromString error");
    return;
  }
  if (!transport->beginTunnel(remote)) {
    NDNOB_LOG_ERR("transport.beginTunnel error");
    return;
  }

  face.reset(new ndnph::Face(*transport));
  gotoState(State::WaitNdncert);
#endif
}

static void
ndncertCallback(void*, ndnph::Data cert)
{
  GotoState gotoState;

  if (!cert) {
    NDNOB_LOG_ERR("ndncert failed");
    return;
  }

  oCert = oRegion.create<ndnph::Data>();
  if (!oCert || !oCert.decodeFrom(cert) || !ndnph::ec::isCertificate(oCert)) {
    NDNOB_LOG_ERR("issued cert error");
    return;
  }
  pvt.setName(oCert.getName());
  gotoState(State::Success);
}

static bool
initNdncert()
{
  auto pakeDevice = getPakeDevice();
  oRegion.reset();
  if (!ndnph::ec::generate(oRegion, pakeDevice->getDeviceName(), pvt, pub)) {
    NDNOB_LOG_ERR("ec::generate error");
    return false;
  }

  challenge.reset(new ndnph::ndncert::client::PossessionChallenge(pakeDevice->getTempCert(),
                                                                  pakeDevice->getTempSigner()));

  ndnph::ndncert::Client::requestCertificate(ndnph::ndncert::Client::Options{
    .face = *face,
    .profile = pakeDevice->getCaProfile(),
    .challenges = { challenge.get() },
    .pub = pub,
    .pvt = pvt,
    .cb = ndncertCallback,
    .ctx = nullptr,
  });
  return true;
}

void
waitNdncert()
{
  face->loop();

  if (challenge == nullptr && !initNdncert()) {
    state = State::Failure;
    return;
  }
}

void
deleteNdncert()
{
  challenge.reset();
}

const ndnph::Data&
getDeviceCert()
{
  return oCert;
}

const ndnph::PrivateKey&
getDeviceSigner()
{
  return pvt;
}

} // namespace ndnob_device_app
