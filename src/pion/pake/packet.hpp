#ifndef PION_PAKE_PACKET_HPP
#define PION_PAKE_PACKET_HPP

#include "../spake2/spake2.hpp"
#include "an.hpp"

namespace pion {
namespace pake {

using Spake2Authenticator = spake2::Context<spake2::Role::Alice>;
using Spake2Device = spake2::Context<spake2::Role::Bob>;

namespace packet_struct {

/**
 * @brief Print a field in hexadecimal format.
 * @pre @c p is a struct in context.
 * @param field field name.
 */
#define PION_PACKET_PRINT_FIELD_HEX(field)                                                         \
  do {                                                                                             \
    os << #field "=";                                                                              \
    for (size_t i = 0; i < sizeof(p.field); ++i) {                                                 \
      char b[3];                                                                                   \
      std::sprintf(b, "%02X", p.field[i]);                                                         \
      os << b;                                                                                     \
    }                                                                                              \
  } while (false)

struct PakeRequest {
  uint8_t spake2pa[Spake2Device::FirstMessageSize];
  ndnph::Name authenticatorCertName;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const PakeRequest& p) {
    os << "PakeRequest(";
    PION_PACKET_PRINT_FIELD_HEX(spake2pa);
    os << ",authenticatorCertName=" << p.authenticatorCertName;
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct PakeResponse {
  uint8_t spake2pb[Spake2Device::FirstMessageSize];
  uint8_t spake2cb[Spake2Device::SecondMessageSize];

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const PakeResponse& p) {
    os << "PakeResponse(";
    PION_PACKET_PRINT_FIELD_HEX(spake2pb);
    os << ",";
    PION_PACKET_PRINT_FIELD_HEX(spake2cb);
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct ConfirmRequest {
  uint8_t spake2ca[Spake2Device::SecondMessageSize];
  ndnph::tlv::Value nc;
  ndnph::Name caProfileName;
  ndnph::Name deviceName;
  uint64_t timestamp;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const ConfirmRequest& p) {
    os << "ConfirmRequest(";
    PION_PACKET_PRINT_FIELD_HEX(spake2ca);
    os << ",nc.size=" << p.nc.size();
    os << ",caProfileName=" << p.caProfileName;
    os << ",deviceName=" << p.deviceName;
    os << ",timestamp=" << p.timestamp;
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct ConfirmResponse {
  ndnph::Data tempCertReq;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const ConfirmResponse& p) {
    os << "ConfirmResponse(";
    os << "tempCertReq=" << p.tempCertReq.getName();
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct CredentialRequest {
  ndnph::Name tempCertName;

#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const CredentialRequest& p) {
    os << "CredentialRequest(";
    os << "tempCertName=" << p.tempCertName;
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

struct CredentialResponse {
#ifdef NDNPH_PRINT_OSTREAM
  friend std::ostream& operator<<(std::ostream& os, const CredentialResponse&) {
    os << "CredentialResponse(";
    return os << ")";
  }
#endif // NDNPH_PRINT_OSTREAM
};

#undef PION_PACKET_PRINT_FIELD_HEX

} // namespace packet_struct

using AesGcm = ndnph::mbedtls::AesGcm<Spake2Device::SharedKeySize * 8>;

using Encrypted =
  ndnph::EncryptedMessage<TT::InitializationVector, AesGcm::IvLen::value, TT::AuthenticationTag,
                          AesGcm::TagLen::value, TT::EncryptedPayload>;

/** @brief Session ID and encryption context. */
class EncryptSession {
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
  bool importKey(const AesGcm::Key& key) {
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
  ndnph::tlv::Value encrypt(ndnph::Region& region, const Arg&... arg) {
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

using InterestLifetime = std::integral_constant<int, 10000>;

} // namespace pake
} // namespace pion

#endif // PION_PAKE_PACKET_HPP
