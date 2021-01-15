#ifndef NDNOB_PAKE_PACKET_HPP
#define NDNOB_PAKE_PACKET_HPP

#include "an.hpp"

namespace ndnob {
namespace pake {
namespace packet_struct {

/**
 * @brief Print a field in hexadecimal format.
 * @pre @c p is a struct in context.
 * @param field field name.
 */
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
  ndnph::Data tempCertReq;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const ConfirmResponse& p)
  {
    os << "ConfirmResponse(";
    os << "tempCertReq=" << p.tempCertReq.getName();
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

/** @brief Session ID and encryption context. */
class EncryptSession
{
public:
  /** @brief Clear state. */
  void end();

  /**
   * @brief Create new session ID.
   * @return whether success.
   */
  bool begin(ndnph::Region& region);

  /**
   * @brief Assign session ID unless it's already assigned.
   * @return whether current packet belongs to the session.
   */
  bool assign(ndnph::Region& region, ndnph::Name name);

  /** @brief Construct Interest name. */
  ndnph::Name makeName(ndnph::Region& region, const ndnph::Component& verb);

  /**
   * @brief Import AES-GCM key.
   * @return whether success.
   */
  bool importKey(const AesGcm::Key& key)
  {
    aes.reset(new AesGcm());
    return aes->import(key);
  }

  /**
   * @brief Encrypt a message.
   * @param region where to allocate memory.
   * @param arg arguments to @c Encoder::prepend function.
   * @return encrypted-message structure.
   */
  template<typename... Arg>
  ndnph::tlv::Value encrypt(ndnph::Region& region, const Arg&... arg)
  {
    ndnph::Encoder inner(region);
    inner.prepend(arg...);
    inner.trim();
    return aes->encrypt<Encrypted>(region, ndnph::tlv::Value(inner), ss.value(), ss.length());
  }

  ndnph::tlv::Value decrypt(ndnph::Region& region, const Encrypted& encrypted);

public:
  ndnph::Component ss;
  std::unique_ptr<AesGcm> aes;
};

ndnph::Name
computeTempSubjectName(ndnph::Region& region, ndnph::Name authenticatorCertName,
                       ndnph::Name deviceName);

using TempCertValidity = std::integral_constant<int, 300>;

} // namespace pake
} // namespace ndnob

#endif // NDNOB_PAKE_PACKET_HPP
