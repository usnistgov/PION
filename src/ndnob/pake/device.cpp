#include "device.hpp"

namespace ndnob {
namespace pake {

static mbed::Object<mbedtls_entropy_context, mbedtls_entropy_init, mbedtls_entropy_free> entropy;

class Device::GotoState
{
public:
  explicit GotoState(Device* device)
    : m_device(device)
  {}

  bool operator()(State state)
  {
    m_device->m_state = state;
    if (state == State::Success || state == State::Failure) {
      m_device->finishSession();
    }
    m_set = true;
    return true;
  }

  ~GotoState()
  {
    if (!m_set) {
      operator()(State::Failure);
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
      ndnph::EvDecoder::def<TT::Spake2PA>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2pa)) {
          std::copy_n(d.value, d.length, spake2pa);
          return true;
        }
        return false;
      }),
      ndnph::EvDecoder::def<TT::AuthenticatorCertName>([this](const ndnph::Decoder::Tlv& d) {
        return d.vd().decode(authenticatorCertName) &&
               authenticatorCertName[-1].is<ndnph::convention::ImplicitDigest>() &&
               ndnph::certificate::isCertName(authenticatorCertName.getPrefix(-1));
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
        encoder.prependTlv(TT::Spake2PB, ndnph::tlv::Value(spake2pb, sizeof(spake2pb)));
      },
      [this](ndnph::Encoder& encoder) {
        encoder.prependTlv(TT::Spake2CB, ndnph::tlv::Value(spake2cb, sizeof(spake2cb)));
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
      ndnph::EvDecoder::def<TT::Spake2CA>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2ca)) {
          std::copy_n(d.value, d.length, spake2ca);
          return true;
        }
        return false;
      }),
      ndnph::EvDecoder::def<TT::InitializationVector>(&encrypted),
      ndnph::EvDecoder::def<TT::AuthenticationTag>(&encrypted),
      ndnph::EvDecoder::def<TT::EncryptedPayload>(&encrypted));
    return std::make_pair(ok, encrypted);
  }

  bool decrypt(ndnph::Region& region, const Encrypted& encrypted, EncryptSession& session)
  {
    auto inner = session.decrypt(region, encrypted);
    return !!inner &&
           ndnph::EvDecoder::decodeValue(
             inner.makeDecoder(), ndnph::EvDecoder::def<TT::Nc>(&nc),
             ndnph::EvDecoder::def<TT::CaProfileName>([this](const ndnph::Decoder::Tlv& d) {
               return d.vd().decode(caProfileName) &&
                      caProfileName[-1].is<ndnph::convention::ImplicitDigest>();
             }),
             ndnph::EvDecoder::def<TT::DeviceName>(
               [this](const ndnph::Decoder::Tlv& d) { return d.vd().decode(deviceName); }),
             ndnph::EvDecoder::defNni<TT::TimestampNameComponent>(&timestamp));
  }
};

template<typename Cert>
static ndnph::Data::Signed
makeConfirmResponseData(ndnph::Region& region, const ndnph::Name& confirmRequestName,
                        EncryptSession& session, const Cert& tReq)
{
  auto encrypted =
    session.encrypt(region, [&](ndnph::Encoder& encoder) { encoder.prependTlv(TT::TReq, tReq); });

  ndnph::Data data = region.create<ndnph::Data>();
  if (!tReq || !encrypted || !data) {
    return ndnph::Data::Signed();
  }
  data.setName(confirmRequestName);
  data.setContent(encrypted);
  return data.sign(ndnph::NullKey::get());
}

class Device::CredentialRequest : public packet_struct::CredentialRequest
{
public:
  bool fromInterest(ndnph::Region& region, const ndnph::Interest& interest, EncryptSession& session)
  {
    Encrypted encrypted;
    bool ok =
      ndnph::EvDecoder::decodeValue(interest.getAppParameters().makeDecoder(),
                                    ndnph::EvDecoder::def<TT::InitializationVector>(&encrypted),
                                    ndnph::EvDecoder::def<TT::AuthenticationTag>(&encrypted),
                                    ndnph::EvDecoder::def<TT::EncryptedPayload>(&encrypted));
    if (!ok) {
      return false;
    }

    auto inner = session.decrypt(region, encrypted);
    return !!inner && ndnph::EvDecoder::decodeValue(inner.makeDecoder(),
                                                    ndnph::EvDecoder::def<TT::IssuedCertName>(
                                                      [this](const ndnph::Decoder::Tlv& d) {
                                                        return d.vd().decode(tempCertName);
                                                      }));
  }
};

Device::Device(const Options& opts)
  : PacketHandler(opts.face, 192)
  , m_pending(this)
{}

void
Device::end()
{
  finishSession();
  m_state = State::Idle;
  m_oRegion.reset();
}

bool
Device::begin(ndnph::tlv::Value password)
{
  end();
  m_iRegion.reset(new decltype(m_iRegion)::element_type);
  m_oRegion.reset(new decltype(m_oRegion)::element_type);

  uint8_t* passwordCopy = m_iRegion->alloc(password.size());
  if (passwordCopy == nullptr) {
    end();
    return false;
  }
  std::copy(password.begin(), password.end(), passwordCopy);
  m_password = ndnph::tlv::Value(passwordCopy, password.size());

  m_spake2.reset(new spake2::Context<>(spake2::Role::Bob, entropy));
  m_state = State::WaitPakeRequest;
  return true;
}

void
Device::loop()
{
  switch (m_state) {
    case State::FetchCaProfile: {
      sendFetchInterest(m_caProfileName, State::WaitCaProfile);
      break;
    }
    case State::FetchAuthenticatorCert: {
      sendFetchInterest(m_authenticatorCertName, State::WaitAuthenticatorCert);
      break;
    }
    case State::FetchTempCert: {
      sendFetchInterest(m_tempCertName, State::WaitTempCert);
      break;
    }
    case State::WaitCaProfile:
    case State::WaitAuthenticatorCert:
    case State::WaitTempCert: {
      if (m_pending.expired()) {
        m_state = State::Failure;
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
  return name.size() == getLocalhopOnboardingPrefix().size() + 3 &&
         getLocalhopOnboardingPrefix().isPrefixOf(name) && name[-2] == expectedVerb &&
         interest.checkDigest() && m_session.assign(*m_iRegion, interest.getName());
}

void
Device::saveCurrentInterest(ndnph::Interest interest)
{
  m_lastInterestName = interest.getName().clone(*m_iRegion);
  m_lastInterestPacketInfo = *getCurrentPacketInfo();
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
  bool ok =
    req.fromInterest(region, interest) &&
    m_spake2->start(m_password.begin(), m_password.size(), nullptr, 0,
                    req.authenticatorCertName[-1].value(), req.authenticatorCertName[-1].length(),
                    m_session.ss.value(), m_session.ss.length()) &&
    m_spake2->generateFirstMessage(res.spake2pb, sizeof(res.spake2pb)) &&
    m_spake2->processFirstMessage(req.spake2pa, sizeof(req.spake2pa)) &&
    m_spake2->generateSecondMessage(res.spake2cb, sizeof(res.spake2cb)) &&
    reply(res.toData(region, interest)) && gotoState(State::WaitConfirmRequest);

  if (ok) {
    m_authenticatorCertName = req.authenticatorCertName.clone(*m_iRegion);
  }
  return true;
}

bool
Device::handleConfirmRequest(ndnph::Interest interest)
{
  if (!checkInterestVerb(interest, getConfirmComponent())) {
    return false;
  }

  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  ConfirmRequest req;
  bool ok = false;
  Encrypted encrypted;
  std::tie(ok, encrypted) = req.fromInterest(interest);
  ok = ok && m_spake2->processSecondMessage(req.spake2ca, sizeof(req.spake2ca));
  if (!ok) {
    return true;
  }

  ok = m_session.importKey(m_spake2->getSharedKey()) && req.decrypt(region, encrypted, m_session);
  if (!ok) {
    return true;
  }

  ndnph::port::UnixTime::set(req.timestamp);

  saveCurrentInterest(interest);
  m_networkCredential = req.nc.clone(*m_oRegion);
  m_caProfileName = req.caProfileName.clone(*m_iRegion);
  m_deviceName = req.deviceName.clone(*m_oRegion);

  return gotoState(State::FetchCaProfile);
}

bool
Device::handleCredentialRequest(ndnph::Interest interest)
{
  if (!checkInterestVerb(interest, getCredentialComponent())) {
    return false;
  }

  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  CredentialRequest req;
  if (!req.fromInterest(region, interest, m_session)) {
    return true;
  }

  saveCurrentInterest(interest);
  m_tempCertName = req.tempCertName.clone(*m_iRegion);
  return gotoState(State::FetchTempCert);
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
  m_pending.send(interest, WithEndpointId(m_lastInterestPacketInfo.endpointId)) &&
    gotoState(nextState);
}

bool
Device::processData(ndnph::Data data)
{
  if (!m_pending.matchPitToken()) {
    return false;
  }
  switch (m_state) {
    case State::WaitCaProfile: {
      return handleCaProfile(data);
    }
    case State::WaitAuthenticatorCert: {
      return handleAuthenticatorCert(data);
    }
    case State::WaitTempCert: {
      return handleTempCert(data);
    }
    default:
      break;
  }
  return false;
}

bool
Device::handleCaProfile(ndnph::Data data)
{
  if (!m_pending.match(data, m_caProfileName) || !m_caProfile.fromData(*m_oRegion, data)) {
    return false;
  }

  GotoState gotoState(this);
  if (!ndnph::certificate::getValidity(m_caProfile.cert).includes(time(nullptr))) {
    // CA certificate expired
    return true;
  }

  gotoState(State::FetchAuthenticatorCert);
  return true;
}

bool
Device::handleAuthenticatorCert(ndnph::Data data)
{
  if (!m_pending.match(data, m_authenticatorCertName)) {
    return false;
  }

  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  if (!data.verify(m_caProfile.pub) || !ndnph::certificate::getValidity(data).includesUnix()) {
    return false;
  }

  ndnph::Name tSubject = computeTempSubjectName(region, data.getName(), m_deviceName);
  if (!tSubject || !ndnph::ec::generate(*m_oRegion, tSubject, m_tPvt, m_tPub)) {
    return true;
  }

  auto tCert = m_tPub.selfSign(region, ndnph::ValidityPeriod::getMax(), m_tPvt);
  send(makeConfirmResponseData(region, m_lastInterestName, m_session, tCert),
       m_lastInterestPacketInfo) &&
    gotoState(State::WaitCredentialRequest);
  return true;
}

bool
Device::handleTempCert(ndnph::Data data)
{
  if (!m_pending.match(data, m_tempCertName)) {
    return false;
  }

  GotoState gotoState(this);
  ndnph::StaticRegion<2048> region;
  auto res = region.create<ndnph::Data>();

  m_tempCert = m_oRegion->create<ndnph::Data>();
  if (!res || !m_tempCert || !m_tempCert.decodeFrom(data)) {
    return true;
  }
  m_tPvt.setName(m_tempCert.getName());

  res.setName(m_lastInterestName);
  send(res.sign(ndnph::NullKey::get()), m_lastInterestPacketInfo) && gotoState(State::Success);
  return true;
}

void
Device::finishSession()
{
  m_session.end();
  m_spake2.reset();
  m_iRegion.reset();
}

} // namespace pake
} // namespace ndnob
