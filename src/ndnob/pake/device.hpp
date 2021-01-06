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
    WaitCaProfile,
    WaitAuthenticatorCert,
    WaitCredentialRequest,
    WaitCompletion,
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

  bool processInterest(ndnph::Interest interest) final;

  bool checkInterestVerb(ndnph::Interest interest, const ndnph::Component& expectedVerb);

  bool handlePakeRequest(ndnph::Interest interest);

  bool handleConfirmRequest(ndnph::Interest interest);

  bool handleCredentialRequest(ndnph::Interest interest);

  bool processData(ndnph::Data data) final;

private:
  class PakeRequest;
  class PakeResponse;
  class ConfirmRequest;

  State m_state = State::Idle;

  ndnph::DynamicRegion m_region;
  ndnph::Name m_sessionPrefix;
  std::unique_ptr<spake2::Spake2> m_spake2;
  ndnph::port::Clock::Time m_deadline;
  uint64_t m_lastPitToken = 0;
};

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_DEVICE_HPP
