#include "app.hpp"

#if defined(PION_SKIP_PAKE) && !defined(PION_SKIP_NDNCERT)
#error "PION_SKIP_NDNCERT must be defined if PION_SKIP_PAKE is defined"
#endif

namespace pion_device_app {

#if defined(PION_INFRA_UDP)
static std::unique_ptr<esp8266ndn::UdpTransport> transport;
static constexpr size_t NC_ITEMS = 3;
#elif defined(PION_INFRA_ETHER)
static std::unique_ptr<esp8266ndn::EthernetTransport> transport;
static constexpr size_t NC_ITEMS = 2;
#else
#error "need either PION_INFRA_UDP or PION_INFRA_ETHER"
#endif
static std::unique_ptr<ndnph::transport::Tracer> transportTracer;
static std::unique_ptr<ndnph::Face> face;
static ndnph::StaticRegion<2048> oRegion;
static ndnph::EcPrivateKey pvt;
static ndnph::EcPublicKey pub;
static std::unique_ptr<ndnph::ndncert::client::PossessionChallenge> challenge;
static ndnph::Data oCert;
static std::unique_ptr<ndnph::PingServer> pingServer;

void
doInfraConnect()
{
  GotoState gotoState;

  char ncBuf[64] = { 0 };
  std::array<const char*, NC_ITEMS> nc;
  {
#ifndef PION_SKIP_PAKE
    auto ncV = getPakeDevice()->getNetworkCredential();
#else
    auto ncV = ndnph::tlv::Value::fromString(PION_INFRA_NC);
#endif
    if (ncV.size() + 1 >= sizeof(ncBuf)) {
      PION_LOG_ERR("NetworkCredential too long");
      return;
    }
    std::copy(ncV.begin(), ncV.end(), ncBuf);
  }

  for (size_t i = 0; i < nc.size(); ++i) {
    nc[i] = strtok(i == 0 ? ncBuf : nullptr, "\n");
    if (nc[i] == nullptr) {
      PION_LOG_ERR("NetworkCredential[%zu] missing", i);
      return;
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(nc[0], nc[1]);
  WiFi.waitForConnectResult();
  if (!WiFi.isConnected()) {
    PION_LOG_ERR("WiFi.begin error %d", WiFi.status());
    return;
  }

#if defined(PION_INFRA_UDP)
  transport.reset(new esp8266ndn::UdpTransport);
  IPAddress remote;
  if (!remote.fromString(nc[2])) {
    PION_LOG_ERR("IPAddress.fromString error");
    return;
  }
  if (!transport->beginTunnel(remote)) {
    PION_LOG_ERR("transport.beginTunnel error");
    return;
  }
#elif defined(PION_INFRA_ETHER)
  transport.reset(new esp8266ndn::EthernetTransport);
  if (!transport->begin()) {
    PION_LOG_ERR("transport.begin error");
    return;
  }
#endif

  transportTracer.reset(new ndnph::transport::Tracer(*transport, "pion.T.infra"));
  face.reset(new ndnph::Face(*transportTracer));
  gotoState(State::WaitNdncert);
}

#ifndef PION_SKIP_NDNCERT

static void
ndncertCallback(void*, ndnph::Data cert)
{
  GotoState gotoState;

  if (!cert) {
    PION_LOG_ERR("ndncert failed");
    return;
  }

  oCert = oRegion.create<ndnph::Data>();
  if (!oCert || !oCert.decodeFrom(cert) || !ndnph::ec::isCertificate(oCert)) {
    PION_LOG_ERR("issued cert error");
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
    PION_LOG_ERR("ec::generate error");
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

#endif // PION_SKIP_NDNCERT

void
waitNdncert()
{
  face->loop();

#ifndef PION_SKIP_NDNCERT
  if (challenge == nullptr && !initNdncert()) {
    state = State::Failure;
    return;
  }
#else
  ndnph::ec::generate(oRegion, ndnph::Name::parse(oRegion, PION_PING_NAME), pvt, pub);
  state = State::Failure;
#endif
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

static void
initPingServer()
{
  pingServer.reset(
    new ndnph::PingServer(ndnph::certificate::toSubjectName(oRegion, pvt.getName()), *face, pvt));
}

void
runPingServer()
{
  if (pingServer == nullptr) {
    initPingServer();
  }
  face->loop();
}

} // namespace pion_device_app
