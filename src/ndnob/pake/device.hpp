#ifndef NDNOB_PAKE_DEVICE_HPP
#define NDNOB_PAKE_DEVICE_HPP

#include "packet.hpp"

namespace ndnob {
namespace pake {

/** @brief NDN onboarding protocol - PAKE stage, device side. */
class Device : public ndnph::PacketHandler
{
public:
  struct Options
  {
    /** @brief Face for communication. */
    ndnph::Face& face;
  };

  explicit Device(const Options& opts);

  void end();

  bool begin(ndnph::tlv::Value password);

  enum class State
  {
    Idle,
    WaitPakeRequest,
    WaitConfirmRequest,
    FetchCaProfile,
    WaitCaProfile,
    FetchAuthenticatorCert,
    WaitAuthenticatorCert,
    WaitCredentialRequest,
    PendingCompletion,
    Success,
    Failure,
  };

  State getState() const
  {
    return m_state;
  }

  // XXX temporary
  const ndnph::Name& getDeviceName() const
  {
    return m_deviceName;
  }

private:
  void loop() final;

  bool processInterest(ndnph::Interest interest) final;

  bool checkInterestVerb(ndnph::Interest interest, const ndnph::Component& expectedVerb);

  bool handlePakeRequest(ndnph::Interest interest);

  bool handleConfirmRequest(ndnph::Interest interest);

  bool handleCredentialRequest(ndnph::Interest interest);

  void sendFetchInterest(const ndnph::Name& name, State nextState);

  bool processData(ndnph::Data data) final;

  bool handleCaProfile(ndnph::Data data);

  bool handleAuthenticatorCert(ndnph::Data data);

private:
  class GotoState;
  class PakeRequest;
  class PakeResponse;
  class ConfirmRequest;

  State m_state = State::Idle;
  ndnph::port::Clock::Time m_deadline;
  uint64_t m_lastPitToken = 0;

  ndnph::DynamicRegion m_region;
  ndnph::Name m_sessionPrefix;
  std::unique_ptr<spake2::Spake2> m_spake2;
  std::unique_ptr<AesGcm> m_aes;
  ndnph::EcPrivateKey m_tPvt;
  ndnph::EcPublicKey m_tPub;

  ndnph::Name m_confirmRequestInterestName;
  PacketInfo m_confirmRequestPacketInfo;
  ndnph::Name m_caProfileName;
  ndnph::Name m_authenticatorCertName;
  ndnph::Name m_deviceName;
  ndnph::ndncert::client::CaProfile m_caProfile;
};

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_DEVICE_HPP
