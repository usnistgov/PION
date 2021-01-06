#include "device.hpp"

namespace ndnob {
namespace pake {

static constexpr int REQUEST_DEADLINE = 2000;
static constexpr int COMPLETE_DEADLINE = 6000;

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
  bool fromInterest(ndnph::Region&, const ndnph::Interest& interest)
  {
    return ndnph::EvDecoder::decodeValue(
      interest.getAppParameters().makeDecoder(),
      ndnph::EvDecoder::def<TT::Spake2Fkca>([this](const ndnph::Decoder::Tlv& d) {
        if (d.length == sizeof(spake2Fkca)) {
          std::copy_n(d.value, d.length, spake2Fkca);
          return true;
        }
        return false;
      }));
  }
};

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
  if (!m_spake2->start(password.begin(), password.size())) {
    return false;
  }

  m_state = State::WaitPakeRequest;
  return true;
}

void
Device::loop()
{
  ndnph::StaticRegion<2048> region;
  switch (m_state) {
    case State::WaitCaProfile:
    case State::WaitAuthenticatorCert: {
      if (ndnph::port::Clock::isBefore(m_deadline, ndnph::port::Clock::now())) {
        m_state = State::Failure;
      }
      break;
    }
    case State::WaitCompletion: {
      if (ndnph::port::Clock::isBefore(m_deadline, ndnph::port::Clock::now())) {
        m_state = State::Success;
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
      if (handlePakeRequest(interest)) {
        m_state = State::WaitConfirmRequest;
        return true;
      }
      break;
    }
    case State::WaitConfirmRequest: {
      if (handleConfirmRequest(interest)) {
        m_state = State::WaitCredentialRequest;
        return true;
      }
      break;
    }
    case State::WaitCredentialRequest: {
      if (handleCredentialRequest(interest)) {
        m_state = State::WaitCompletion;
        m_deadline = ndnph::port::Clock::add(ndnph::port::Clock::now(), COMPLETE_DEADLINE);
        return true;
      }
      break;
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
      name[3] != expectedVerb) {
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
  ndnph::StaticRegion<4096> region;
  PakeRequest req;
  PakeResponse res;
  return checkInterestVerb(interest, getPakeComponent()) && req.fromInterest(region, interest) &&
         m_spake2->generateFirstMessage(res.spake2S, sizeof(res.spake2S)) &&
         m_spake2->processFirstMessage(req.spake2T, sizeof(req.spake2T)) &&
         m_spake2->generateSecondMessage(res.spake2Fkcb, sizeof(res.spake2Fkcb)) &&
         reply(res.toData(region, interest));
}

bool
Device::handleConfirmRequest(ndnph::Interest interest)
{
  ndnph::StaticRegion<2048> region;
  ConfirmRequest req;
  return checkInterestVerb(interest, getConfirmComponent()) && req.fromInterest(region, interest) &&
         m_spake2->processSecondMessage(req.spake2Fkca, sizeof(req.spake2Fkca));
}
bool
Device::handleCredentialRequest(ndnph::Interest interest)
{
  ndnph::StaticRegion<2048> region;
  return checkInterestVerb(interest, getCredentialComponent());
}

bool Device::processData(ndnph::Data)
{
  return false;
}

} // namespace pake
} // namespace ndnob
