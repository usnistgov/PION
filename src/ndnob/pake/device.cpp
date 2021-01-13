#include "device.hpp"
#include <sys/time.h>

namespace ndnob {
namespace pake {

static constexpr int REQUEST_DEADLINE = 2000;
static constexpr int COMPLETE_DEADLINE = 6000;

class Device::GotoState
{
public:
  explicit GotoState(Device* device)
    : m_device(device)
  {}

  bool operator()(State state, int deadline = REQUEST_DEADLINE)
  {
    m_device->m_state = state;
    m_device->m_deadline = ndnph::port::Clock::add(ndnph::port::Clock::now(), deadline);
    m_set = true;
    return true;
  }

  ~GotoState()
  {
    if (!m_set) {
      m_device->m_state = State::Failure;
    }
  }

private:
  Device* m_device;
  bool m_set = false;
};

class Device::PakeRequest : public packet_struct::PakeRequest
{
public:
  bool fromInterest(ndnph::Region&, const ndnph::Interest& interest)
  {
    return ndnph::EvDecoder::decodeValue(
      interest.getAppParameters().makeDecoder(),
      ndnph::EvDecoder::def<TT::Spake2T>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2T)) {
          std::copy_n(d.value, d.length, spake2T);
          return true;
        }
        return false;
      }));
  }
};

class Device::PakeResponse : public packet_struct::PakeResponse
{
public:
  ndnph::Data::Signed toData(ndnph::Region& region, const ndnph::Interest& pakeRequest) const
  {
    ndnph::Encoder encoder(region);
    encoder.prepend(
      [this](ndnph::Encoder& encoder) {
        encoder.prependTlv(TT::Spake2S, ndnph::tlv::Value(spake2S, sizeof(spake2S)));
      },
      [this](ndnph::Encoder& encoder) {
        encoder.prependTlv(TT::Spake2Fkcb, ndnph::tlv::Value(spake2Fkcb, sizeof(spake2Fkcb)));
      });
    encoder.trim();

    ndnph::Data data = region.create<ndnph::Data>();
    if (!encoder || !data || !pakeRequest) {
      return ndnph::Data::Signed();
    }
    data.setName(pakeRequest.getName());
    data.setContent(ndnph::tlv::Value(encoder));
    return data.sign(ndnph::NullKey::get());
  }
};

class Device::ConfirmRequest : public packet_struct::ConfirmRequest
{
public:
  std::pair<bool, Encrypted> fromInterest(const ndnph::Interest& interest)
  {
    Encrypted encrypted;
    bool ok = ndnph::EvDecoder::decodeValue(
      interest.getAppParameters().makeDecoder(),
      ndnph::EvDecoder::def<TT::Spake2Fkca>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2Fkca)) {
          std::copy_n(d.value, d.length, spake2Fkca);
          return true;
        }
        return false;
      }),
      ndnph::EvDecoder::def<TT::InitializationVector>(&encrypted),
      ndnph::EvDecoder::def<TT::AuthenticationTag>(&encrypted),
      ndnph::EvDecoder::def<TT::EncryptedPayload>(&encrypted));
    return std::make_pair(ok, encrypted);
  }

  bool decrypt(ndnph::Region& region, const Encrypted& encrypted, const ndnph::Component& session,
               AesGcm& aes)
  {
    ndnph::tlv::Value inner = aes.decrypt(region, encrypted, session.value(), session.length());
    return !!inner &&
           ndnph::EvDecoder::decodeValue(
             inner.makeDecoder(), ndnph::EvDecoder::def<TT::Nc>(&nc),
             ndnph::EvDecoder::def<TT::CaProfileName>(
               [this](const ndnph::Decoder::Tlv& d) { return d.vd().decode(caProfileName); }),
             ndnph::EvDecoder::def<TT::AuthenticatorCertName>([this](const ndnph::Decoder::Tlv& d) {
               return d.vd().decode(authenticatorCertName);
             }),
             ndnph::EvDecoder::def<TT::DeviceName>(
               [this](const ndnph::Decoder::Tlv& d) { return d.vd().decode(deviceName); }),
             ndnph::EvDecoder::defNni<TT::TimestampNameComponent>(&timestamp)) &&
           caProfileName[-1].is<ndnph::convention::ImplicitDigest>() &&
           authenticatorCertName[-1].is<ndnph::convention::ImplicitDigest>();
  }
};

template<typename Cert>
static ndnph::Data::Signed
makeConfirmResponseData(ndnph::Region& region, const ndnph::Name& confirmRequestName,
                        const ndnph::Component& session, AesGcm& aes, const Cert& tReq)
{
  ndnph::Encoder inner(region);
  inner.prependTlv(TT::TReq, tReq);
  inner.trim();
  auto encrypted =
    aes.encrypt<Encrypted>(region, ndnph::tlv::Value(inner), session.value(), session.length());

  ndnph::Data data = region.create<ndnph::Data>();
  if (!tReq || !inner || !data || !encrypted) {
    return ndnph::Data::Signed();
  }
  data.setName(confirmRequestName);
  data.setContent(encrypted);
  return data.sign(ndnph::NullKey::get());
}

Device::Device(const Options& opts)
  : PacketHandler(opts.face, 192)
  , m_region(4096)
{}

void
Device::end()
{
  m_spake2.reset();
  m_state = State::Idle;
  m_sessionPrefix = ndnph::Name();
  m_region.reset();
}

bool
Device::begin(ndnph::tlv::Value password)
{
  end();

  mbed::Object<mbedtls_entropy_context, mbedtls_entropy_init, mbedtls_entropy_free> entropy;
  m_spake2.reset(new spake2::Spake2(spake2::Role::Bob, entropy));
  if (!m_spake2->start(password.begin(), password.size()) ||
      !ndnph::port::RandomSource::generate(reinterpret_cast<uint8_t*>(&m_lastPitToken),
                                           sizeof(m_lastPitToken))) {
    return false;
  }

  m_state = State::WaitPakeRequest;
  return true;
}

void
Device::loop()
{
  State timeoutState = State::Failure;
  switch (m_state) {
    case State::FetchCaProfile: {
      sendFetchInterest(m_caProfileName, State::WaitCaProfile);
      break;
    }
    case State::FetchAuthenticatorCert: {
      sendFetchInterest(m_authenticatorCertName, State::WaitAuthenticatorCert);
      break;
    }
    case State::PendingCompletion:
      timeoutState = State::Success;
      // fallthrough
    case State::WaitCaProfile:
    case State::WaitAuthenticatorCert: {
      if (ndnph::port::Clock::isBefore(m_deadline, ndnph::port::Clock::now())) {
        m_state = timeoutState;
      }
      break;
    }
    default:
      break;
  }
}

bool
Device::processInterest(ndnph::Interest interest)
{
#ifdef ARDUINO
  Serial.print(">I ");
  Serial.print(interest.getName());
  Serial.print(" ");
  Serial.println(interest.getAppParameters().size());
#endif
  switch (m_state) {
    case State::WaitPakeRequest: {
      return handlePakeRequest(interest);
    }
    case State::WaitConfirmRequest: {
      return handleConfirmRequest(interest);
    }
    case State::WaitCredentialRequest: {
      return handleCredentialRequest(interest);
    }
    default:
      break;
  }
  return false;
}

bool
Device::checkInterestVerb(ndnph::Interest interest, const ndnph::Component& expectedVerb)
{
  const auto& name = interest.getName();
  if (name.size() != 5 || !getLocalhopOnboardingPrefix().isPrefixOf(name) ||
      name[3] != expectedVerb || !interest.checkDigest()) {
    return false;
  }
  if (!m_sessionPrefix) {
    m_sessionPrefix = name.getPrefix(3).clone(m_region);
  }
  return m_sessionPrefix.isPrefixOf(name);
}

bool
Device::handlePakeRequest(ndnph::Interest interest)
{
  if (!checkInterestVerb(interest, getPakeComponent())) {
    return false;
  }

  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  PakeRequest req;
  PakeResponse res;
  req.fromInterest(region, interest) &&
    m_spake2->generateFirstMessage(res.spake2S, sizeof(res.spake2S)) &&
    m_spake2->processFirstMessage(req.spake2T, sizeof(req.spake2T)) &&
    m_spake2->generateSecondMessage(res.spake2Fkcb, sizeof(res.spake2Fkcb)) &&
    reply(res.toData(region, interest)) && gotoState(State::WaitConfirmRequest);
  return true;
}

bool
Device::handleConfirmRequest(ndnph::Interest interest)
{
  if (!checkInterestVerb(interest, getConfirmComponent())) {
    return false;
  }

  GotoState gotoState(this);
  ConfirmRequest req;
  bool ok = false;
  Encrypted encrypted;
  std::tie(ok, encrypted) = req.fromInterest(interest);
  ok = ok && m_spake2->processSecondMessage(req.spake2Fkca, sizeof(req.spake2Fkca));
  if (!ok) {
    return true;
  }

  m_aes.reset(new AesGcm());
  ok = m_aes->import(m_spake2->getSharedKey()) &&
       req.decrypt(m_region, encrypted, m_sessionPrefix[-1], *m_aes);
  if (!ok) {
    return true;
  }

#ifdef ARDUINO
  timeval tv = {
    .tv_sec = static_cast<long>(req.timestamp / ndnph::convention::TimeValue::Microseconds),
  };
  settimeofday(&tv, nullptr);
#endif

  // copy to session region because these are needed later
  m_confirmRequestInterestName = interest.getName().clone(m_region);
  m_confirmRequestPacketInfo = *getCurrentPacketInfo();
  // the names are already in session region
  m_caProfileName = req.caProfileName;
  m_authenticatorCertName = req.authenticatorCertName;
  m_deviceName = req.deviceName;

  return gotoState(State::FetchCaProfile);
}

bool
Device::handleCredentialRequest(ndnph::Interest interest)
{
  GotoState gotoState(this);
  bool ok = checkInterestVerb(interest, getCredentialComponent());
  if (!ok) {
    return false;
  }

  return gotoState(State::PendingCompletion, COMPLETE_DEADLINE);
}

void
Device::sendFetchInterest(const ndnph::Name& name, State nextState)
{
  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  auto interest = region.create<ndnph::Interest>();
  if (!interest) {
    return;
  }
  interest.setName(name);
  send(interest, WithEndpointId(m_confirmRequestPacketInfo.endpointId),
       WithPitToken(++m_lastPitToken)) &&
    gotoState(nextState);
}

bool
Device::processData(ndnph::Data data)
{
  if (getCurrentPacketInfo()->pitToken != m_lastPitToken) {
    return false;
  }
  switch (m_state) {
    case State::WaitCaProfile: {
      return handleCaProfile(data);
    }
    case State::WaitAuthenticatorCert: {
      return handleAuthenticatorCert(data);
    }
    default:
      break;
  }
  return false;
}

bool
Device::handleCaProfile(ndnph::Data data)
{
  ndnph::StaticRegion<2048> region;
  if (data.getFullName(region) != m_caProfileName || !m_caProfile.fromData(m_region, data)) {
    return false;
  }
  region.reset();

  GotoState gotoState(this);
  // TODO check CA certificate is unexpired

  gotoState(State::FetchAuthenticatorCert);
  return true;
}

bool
Device::handleAuthenticatorCert(ndnph::Data data)
{
  ndnph::StaticRegion<2048> region;
  if (data.getFullName(region) != m_authenticatorCertName) {
    return false;
  }
  region.reset();

  GotoState gotoState(this);
  if (!data.verify(m_caProfile.pub)) {
    return false;
  }
  // TODO check authenticator certificate is unexpired

  ndnph::Name tSubject = computeTempSubjectName(region, data.getName(), m_deviceName);
  if (!tSubject || !ndnph::ec::generate(region, tSubject, m_tPvt, m_tPub)) {
    return true;
  }

  auto tCert = m_tPub.selfSign(region, ndnph::ValidityPeriod::getMax(), m_tPvt);
  send(makeConfirmResponseData(region, m_confirmRequestInterestName, m_sessionPrefix[-1], *m_aes,
                               tCert),
       m_confirmRequestPacketInfo) &&
    gotoState(State::WaitCredentialRequest);
  return true;
}

} // namespace pake
} // namespace ndnob
