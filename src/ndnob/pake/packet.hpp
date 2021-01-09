#ifndef NDNOB_PAKE_PACKET_HPP
#define NDNOB_PAKE_PACKET_HPP

#include "an.hpp"

namespace ndnob {
namespace pake {
namespace packet_struct {

#define NDNOB_PACKET_PRINT_FIELD_HEX(field)                                                        \
  do {                                                                                             \
    os << #field "=";                                                                              \
    for (size_t i = 0; i < sizeof(p.field); ++i) {                                                 \
      char b[3];                                                                                   \
      std::sprintf(b, "%02X", p.field[i]);                                                         \
      os << b;                                                                                     \
    }                                                                                              \
  } while (false)

struct PakeRequest
{
  uint8_t spake2T[65];

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const PakeRequest& p)
  {
    os << "PakeRequest(";
    NDNOB_PACKET_PRINT_FIELD_HEX(spake2T);
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct PakeResponse
{
  uint8_t spake2S[65];
  uint8_t spake2Fkcb[32];

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const PakeResponse& p)
  {
    os << "PakeResponse(";
    NDNOB_PACKET_PRINT_FIELD_HEX(spake2S);
    os << ",";
    NDNOB_PACKET_PRINT_FIELD_HEX(spake2Fkcb);
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct ConfirmRequest
{
  uint8_t spake2Fkca[32];
  ndnph::tlv::Value nc;
  ndnph::Name caProfileName;
  ndnph::Name authenticatorCertName;
  ndnph::Name deviceName;
  uint64_t timestamp;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const ConfirmRequest& p)
  {
    os << "ConfirmRequest(";
    NDNOB_PACKET_PRINT_FIELD_HEX(spake2Fkca);
    os << ",nc.size=" << p.nc.size();
    os << ",caProfileName=" << p.caProfileName;
    os << ",authenticatorCertName=" << p.authenticatorCertName;
    os << ",deviceName=" << p.deviceName;
    os << ",timestamp=" << p.timestamp;
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct ConfirmResponse
{
  ndnph::tlv::Value tempCertReq;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const ConfirmResponse& p)
  {
    os << "ConfirmResponse(";
    os << "tempCertReq.size=" << p.tempCertReq.size();
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct CredentialRequest
{
  ndnph::Name tempCertName;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const CredentialRequest& p)
  {
    os << "CredentialRequest(";
    os << "tempCertName=" << p.tempCertName;
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct CredentialResponse
{
#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const CredentialResponse&)
  {
    os << "CredentialResponse(";
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

#undef NDNOB_PACKET_PRINT_FIELD_HEX

} // namespace packet_struct

using AesGcm = ndnph::mbedtls::AesGcm<128>;

using Encrypted =
  ndnph::EncryptedMessage<TT::InitializationVector, AesGcm::IvLen::value, TT::AuthenticationTag,
                          AesGcm::TagLen::value, TT::EncryptedPayload>;

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_PACKET_HPP
