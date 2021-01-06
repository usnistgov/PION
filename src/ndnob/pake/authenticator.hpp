#ifndef NDNOB_PAKE_AUTHENTICATOR_HPP
#define NDNOB_PAKE_AUTHENTICATOR_HPP

#include "packet.hpp"

namespace ndnob {
namespace pake {

/** @brief NDN onboarding protocol - PAKE stage, authenticator side. */
class Authenticator : public ndnph::PacketHandler
{
public:
  struct Options
  {
    /** @brief Face for communication. */
    ndnph::Face& face;

    /** @brief CA profile packet. */
    ndnph::Data caProfile;

    /** @brief Authenticator certificate. */
    ndnph::Data cert;

    /** @brief Authenticator private key. */
    const ndnph::EcPrivateKey& pvt;
  };

  explicit Authenticator(const Options& opts);

  void end();

  bool begin(ndnph::tlv::Value password);

  enum class State
  {
    Idle,
    SendPakeRequest,
    WaitPakeResponse,
    WaitConfirmResponse,
    SendCredentialRequest,
    WaitCredentialResponse,
    Success,
    Failure,
  };

  State getState() const
  {
    return m_state;
  }

  // XXX temporary
  const std::array<uint8_t, 16>& getKey() const
  {
    return m_spake2->getSharedKey();
  }

private:
  void loop() final;

  bool processData(ndnph::Data data) final;

  bool sendPakeRequest();

  bool handlePakeResponse(ndnph::Data data);

  bool processInterest(ndnph::Interest interest) final;

private:
  class PakeRequest;
  class PakeResponse;
  class ConfirmRequest;

  ndnph::Data m_caProfile;
  ndnph::Data m_cert;
  const ndnph::EcPrivateKey& m_pvt;

  State m_state = State::Idle;

  ndnph::DynamicRegion m_region;
  ndnph::Component m_session;
  std::unique_ptr<spake2::Spake2> m_spake2;
  ndnph::port::Clock::Time m_deadline;
  uint64_t m_lastPitToken = 0;
};

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_AUTHENTICATOR_HPP
