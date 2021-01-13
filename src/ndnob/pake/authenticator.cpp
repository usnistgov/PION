#include "authenticator.hpp"

namespace ndnob {
namespace pake {

static constexpr int REQUEST_DEADLINE = 4000;

class Authenticator::GotoState
{
public:
  explicit GotoState(Authenticator* authenticator)
    : m_authenticator(authenticator)
  {}

  bool operator()(State state, int deadline = REQUEST_DEADLINE)
  {
    m_authenticator->m_state = state;
    m_authenticator->m_deadline = ndnph::port::Clock::add(ndnph::port::Clock::now(), deadline);
    m_set = true;
    return true;
  }

  ~GotoState()
  {
    if (!m_set) {
      m_authenticator->m_state = State::Failure;
    }
  }

private:
  Authenticator* m_authenticator;
  bool m_set = false;
};

class Authenticator::PakeRequest : public packet_struct::PakeRequest
{
public:
  ndnph::Interest::Parameterized toInterest(ndnph::Region& region,
                                            const ndnph::Component& session) const
  {
    ndnph::Encoder encoder(region);
    encoder.prependTlv(TT::Spake2T, ndnph::tlv::Value(spake2T, sizeof(spake2T)));
    encoder.trim();

    ndnph::Name name =
      getLocalhopOnboardingPrefix().append(region, { session, getPakeComponent() });
    ndnph::Interest interest = region.create<ndnph::Interest>();
    if (!encoder || !name || !interest) {
      return ndnph::Interest::Parameterized();
    }
    interest.setName(name);
    return interest.parameterize(ndnph::tlv::Value(encoder));
  }
};

class Authenticator::PakeResponse : public packet_struct::PakeResponse
{
public:
  bool fromData(ndnph::Region&, const ndnph::Data& data)
  {
    return ndnph::EvDecoder::decodeValue(
      data.getContent().makeDecoder(),
      ndnph::EvDecoder::def<TT::Spake2S>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2S)) {
          std::copy_n(d.value, d.length, spake2S);
          return true;
        }
        return false;
      }),
      ndnph::EvDecoder::def<TT::Spake2Fkcb>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2Fkcb)) {
          std::copy_n(d.value, d.length, spake2Fkcb);
          return true;
        }
        return false;
      }));
  }
};

class Authenticator::ConfirmRequest : public packet_struct::ConfirmRequest
{
public:
  ndnph::Interest::Parameterized toInterest(ndnph::Region& region, const ndnph::Component& session,
                                            AesGcm& aes) const
  {
    ndnph::Encoder inner(region);
    inner.prepend(
      [this](ndnph::Encoder& encoder) { encoder.prependTlv(TT::Nc, nc); },
      [this](ndnph::Encoder& encoder) { encoder.prependTlv(TT::CaProfileName, caProfileName); },
      [this](ndnph::Encoder& encoder) {
        encoder.prependTlv(TT::AuthenticatorCertName, authenticatorCertName);
      },
      [this](ndnph::Encoder& encoder) { encoder.prependTlv(TT::DeviceName, deviceName); },
      [&](ndnph::Encoder& encoder) {
        encoder.prepend(
          ndnph::convention::Timestamp::create(region, ndnph::convention::TimeValue()));
      });
    inner.trim();
    auto encrypted =
      aes.encrypt<Encrypted>(region, ndnph::tlv::Value(inner), session.value(), session.length());

    ndnph::Encoder outer(region);
    outer.prepend(
      [this](ndnph::Encoder& encoder) {
        encoder.prependTlv(TT::Spake2Fkca, ndnph::tlv::Value(spake2Fkca, sizeof(spake2Fkca)));
      },
      encrypted);
    outer.trim();

    ndnph::Name name =
      getLocalhopOnboardingPrefix().append(region, { session, getConfirmComponent() });
    ndnph::Interest interest = region.create<ndnph::Interest>();
    if (!inner || !encrypted || !outer || !name || !interest) {
      return ndnph::Interest::Parameterized();
    }
    interest.setName(name);
    return interest.parameterize(ndnph::tlv::Value(outer));
  }
};

class Authenticator::ConfirmResponse
{
public:
  bool fromData(ndnph::Region& region, const ndnph::Data& data, const ndnph::Component& session,
                AesGcm& aes)
  {
    Encrypted encrypted;
    bool ok = ndnph::EvDecoder::decodeValue(
      data.getContent().makeDecoder(), ndnph::EvDecoder::def<TT::InitializationVector>(&encrypted),
      ndnph::EvDecoder::def<TT::AuthenticationTag>(&encrypted),
      ndnph::EvDecoder::def<TT::EncryptedPayload>(&encrypted));
    if (!ok) {
      return false;
    }

    ndnph::tlv::Value inner = aes.decrypt(region, encrypted, session.value(), session.length());
    return !!inner && ndnph::EvDecoder::decodeValue(
                        inner.makeDecoder(),
                        ndnph::EvDecoder::def<TT::TReq>([&](const ndnph::Decoder::Tlv& d) {
                          tempCertReq = region.create<ndnph::Data>();
                          return !!tempCertReq && d.vd().decode(tempCertReq) &&
                                 tPub.import(region, tempCertReq);
                        }));
  }

public:
  ndnph::Data tempCertReq;
  ndnph::EcPublicKey tPub;
};

Authenticator::Authenticator(const Options& opts)
  : PacketHandler(opts.face, 192)
  , m_caProfile(opts.caProfile)
  , m_cert(opts.cert)
  , m_pvt(opts.pvt)
  , m_nc(opts.nc)
  , m_deviceName(opts.deviceName)
  , m_region(4096)
{}

void
Authenticator::end()
{
  m_spake2.reset();
  m_state = State::Idle;
  m_region.reset();
}

bool
Authenticator::begin(ndnph::tlv::Value password)
{
  end();

  uint8_t session[8];
  if (!ndnph::port::RandomSource::generate(session, sizeof(session)) ||
      !ndnph::port::RandomSource::generate(reinterpret_cast<uint8_t*>(&m_lastPitToken),
                                           sizeof(m_lastPitToken))) {
    return false;
  }
  m_session = ndnph::Component(m_region, sizeof(session), session);

  mbed::Object<mbedtls_entropy_context, mbedtls_entropy_init, mbedtls_entropy_free> entropy;
  m_spake2.reset(new spake2::Spake2(spake2::Role::Alice, entropy));
  if (!m_spake2->start(password.begin(), password.size())) {
    return false;
  }

  m_state = State::SendPakeRequest;
  return true;
}

void
Authenticator::loop()
{
  switch (m_state) {
    case State::SendPakeRequest: {
      sendPakeRequest();
      break;
    }
    case State::SendCredentialRequest: {
      break;
    }
    case State::WaitPakeResponse:
    case State::WaitConfirmResponse:
    case State::WaitCredentialResponse: {
      if (ndnph::port::Clock::isBefore(m_deadline, ndnph::port::Clock::now())) {
        m_state = State::Failure;
      }
      break;
    }
    default:
      break;
  }
}

bool
Authenticator::processData(ndnph::Data data)
{
  if (getCurrentPacketInfo()->pitToken != m_lastPitToken) {
    return false;
  }
  switch (m_state) {
    case State::WaitPakeResponse: {
      return handlePakeResponse(data);
    }
    case State::WaitConfirmResponse: {
      return handleConfirmResponse(data);
    }
    case State::WaitCredentialResponse: {
      break;
    }
    default:
      break;
  }
  return false;
}

void
Authenticator::sendPakeRequest()
{
  ndnph::StaticRegion<2048> region;
  GotoState gotoState(this);
  PakeRequest req;
  m_spake2->generateFirstMessage(req.spake2T, sizeof(req.spake2T)) &&
    send(req.toInterest(region, m_session), WithPitToken(++m_lastPitToken)) &&
    gotoState(State::WaitPakeResponse);
}

bool
Authenticator::handlePakeResponse(ndnph::Data data)
{
  ndnph::StaticRegion<2048> region;
  PakeResponse res;
  if (!res.fromData(region, data)) {
    return false;
  }

  GotoState gotoState(this);
  ConfirmRequest req;
  m_aes.reset(new AesGcm());
  bool ok = m_spake2->processFirstMessage(res.spake2S, sizeof(res.spake2S)) &&
            m_spake2->generateSecondMessage(req.spake2Fkca, sizeof(req.spake2Fkca)) &&
            m_spake2->processSecondMessage(res.spake2Fkcb, sizeof(res.spake2Fkcb)) &&
            m_aes->import(m_spake2->getSharedKey());
  m_spake2.reset();
  if (!ok) {
    return true;
  }

  req.nc = m_nc;
  req.caProfileName = m_caProfile.getFullName(region);
  req.authenticatorCertName = m_cert.getFullName(region);
  req.deviceName = m_deviceName;
  // req.timestamp is ignored; current timestamp will be used
  ok = send(req.toInterest(region, m_session, *m_aes), WithPitToken(++m_lastPitToken));
  if (ok) {
    gotoState(State::WaitConfirmResponse);
  }
  return true;
}

bool
Authenticator::handleConfirmResponse(ndnph::Data data)
{
  ndnph::StaticRegion<2048> region;
  ConfirmResponse res;
  if (!res.fromData(region, data, m_session, *m_aes)) {
    return false;
  }
#ifndef ARDUINO
  std::cerr << "tempCertReq.name=" << res.tempCertReq.getName() << std::endl;
#endif

  GotoState gotoState(this);
  return gotoState(State::SendCredentialRequest);
}

bool
Authenticator::processInterest(ndnph::Interest interest)
{
  if (interest.match(m_caProfile)) {
    return reply(m_caProfile);
  }
  if (interest.match(m_cert)) {
    return reply(m_cert);
  }
  return false;
}

} // namespace pake
} // namespace ndnob
