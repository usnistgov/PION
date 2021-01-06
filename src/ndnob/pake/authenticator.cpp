#include "authenticator.hpp"

namespace ndnob {
namespace pake {

static constexpr int REQUEST_DEADLINE = 2000;

class Authenticator::PakeRequest : public packet_struct::PakeRequest
{
public:
  ndnph::Interest::Parameterized toInterest(ndnph::Region& region, const ndnph::Component& session)
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
  ndnph::Interest::Parameterized toInterest(ndnph::Region& region, const ndnph::Component& session)
  {
    ndnph::Encoder encoder(region);
    // TODO encrypted-message portion
    encoder.prependTlv(TT::Spake2Fkca, ndnph::tlv::Value(spake2Fkca, sizeof(spake2Fkca)));
    encoder.trim();

    ndnph::Name name =
      getLocalhopOnboardingPrefix().append(region, { session, getConfirmComponent() });
    ndnph::Interest interest = region.create<ndnph::Interest>();
    if (!encoder || !name || !interest) {
      return ndnph::Interest::Parameterized();
    }
    interest.setName(name);
    return interest.parameterize(ndnph::tlv::Value(encoder));
  }
};

Authenticator::Authenticator(const Options& opts)
  : PacketHandler(opts.face, 192)
  , m_caProfile(opts.caProfile)
  , m_cert(opts.cert)
  , m_pvt(opts.pvt)
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
      if (sendPakeRequest()) {
        m_state = State::WaitPakeResponse;
        m_deadline = ndnph::port::Clock::add(ndnph::port::Clock::now(), REQUEST_DEADLINE);
      } else {
        m_state = State::Failure;
      }
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
      if (handlePakeResponse(data)) {
        m_state = State::WaitConfirmResponse;
        m_deadline = ndnph::port::Clock::add(ndnph::port::Clock::now(), REQUEST_DEADLINE);
      } else {
        m_state = State::Failure;
      }
      break;
    }
    case State::WaitConfirmResponse: {
      break;
    }
    case State::WaitCredentialResponse: {
      break;
    }
    default:
      break;
  }
  return false;
}

bool
Authenticator::sendPakeRequest()
{
  ndnph::StaticRegion<2048> region;
  PakeRequest req;
  return m_spake2->generateFirstMessage(req.spake2T, sizeof(req.spake2T)) &&
         send(req.toInterest(region, m_session), WithPitToken(++m_lastPitToken));
}

bool
Authenticator::handlePakeResponse(ndnph::Data data)
{
  ndnph::StaticRegion<2048> region;
  PakeResponse res;
  ConfirmRequest req;
  return res.fromData(region, data) &&
         m_spake2->processFirstMessage(res.spake2S, sizeof(res.spake2S)) &&
         m_spake2->generateSecondMessage(req.spake2Fkca, sizeof(req.spake2Fkca)) &&
         m_spake2->processSecondMessage(res.spake2Fkcb, sizeof(res.spake2Fkcb)) &&
         send(req.toInterest(region, m_session), WithPitToken(++m_lastPitToken));
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
