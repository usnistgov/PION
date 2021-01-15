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

    /** @brief Network credential to be passed to the device. */
    ndnph::tlv::Value nc;

    /** @brief Assigned device name. */
    ndnph::Name deviceName;
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

private:
  void loop() final;

  bool processData(ndnph::Data data) final;

  void sendPakeRequest();

  bool handlePakeResponse(ndnph::Data data);

  bool handleConfirmResponse(ndnph::Data data);

  void sendCredentialRequest();

  bool processInterest(ndnph::Interest interest) final;

private:
  class GotoState;
  class PakeRequest;
  class PakeResponse;
  class ConfirmRequest;
  class ConfirmResponse;
  class CredentialRequest;

  ndnph::Data m_caProfile;
  ndnph::Data m_cert;
  const ndnph::EcPrivateKey& m_pvt;
  ndnph::tlv::Value m_nc;
  ndnph::Name m_deviceName;

  State m_state = State::Idle;
  ndnph::port::Clock::Time m_deadline;
  uint64_t m_lastPitToken = 0;

  ndnph::DynamicRegion m_region;
  EncryptSession m_session;
  std::unique_ptr<spake2::Spake2> m_spake2;
  ndnph::Data m_issued;
};

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_AUTHENTICATOR_HPP
