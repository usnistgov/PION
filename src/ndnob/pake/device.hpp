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
    FetchTempCert,
    WaitTempCert,
    Success,
    Failure,
  };

  State getState() const
  {
    return m_state;
  }

  const ndnph::ndncert::client::CaProfile& getCaProfile() const
  {
    assert(m_state == State::Success);
    return m_caProfile;
  }

  const ndnph::Name& getDeviceName() const
  {
    assert(m_state == State::Success);
    return m_deviceName;
  }

  const ndnph::Data& getTempCert() const
  {
    assert(m_state == State::Success);
    return m_tempCert;
  }

  const ndnph::PrivateKey& getTempSigner() const
  {
    assert(m_state == State::Success);
    return m_tPvt;
  }

private:
  void loop() final;

  bool processInterest(ndnph::Interest interest) final;

  bool checkInterestVerb(ndnph::Interest interest, const ndnph::Component& expectedVerb);

  void saveCurrentInterest(ndnph::Interest interest);

  bool handlePakeRequest(ndnph::Interest interest);

  bool handleConfirmRequest(ndnph::Interest interest);

  bool handleCredentialRequest(ndnph::Interest interest);

  void sendFetchInterest(const ndnph::Name& name, State nextState);

  bool processData(ndnph::Data data) final;

  bool handleCaProfile(ndnph::Data data);

  bool handleAuthenticatorCert(ndnph::Data data);

  bool handleTempCert(ndnph::Data data);

  void finishSession();

private:
  class GotoState;
  class PakeRequest;
  class PakeResponse;
  class ConfirmRequest;
  class CredentialRequest;

  OutgoingPendingInterest m_pending;
  State m_state = State::Idle;
  std::unique_ptr<ndnph::StaticRegion<2048>> m_iRegion; // for intermediate values
  std::unique_ptr<ndnph::StaticRegion<2048>> m_oRegion; // for output values

  ndnph::tlv::Value m_password;
  EncryptSession m_session;
  std::unique_ptr<spake2::Context<>> m_spake2;

  ndnph::Name m_lastInterestName;
  PacketInfo m_lastInterestPacketInfo;
  ndnph::Name m_authenticatorCertName;
  ndnph::Name m_caProfileName;
  ndnph::Name m_tempCertName;

  ndnph::EcPrivateKey m_tPvt;
  ndnph::EcPublicKey m_tPub;
  ndnph::tlv::Value m_networkCredential;
  ndnph::ndncert::client::CaProfile m_caProfile;
  ndnph::Name m_deviceName;
  ndnph::Data m_tempCert;
};

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_DEVICE_HPP
