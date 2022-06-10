#ifndef PION_PAKE_AN_HPP
#define PION_PAKE_AN_HPP

#include "../common.hpp"

namespace pion {
namespace pake {

/** @brief TLV-TYPE assigned numbers. */
namespace TT {
enum
{
  Spake2PA = 0x8F01,
  AuthenticatorCertName = 0x8F0D,
  Spake2PB = 0x8F03,
  Spake2CB = 0x8F05,
  Spake2CA = 0x8F07,
  Nc = 0x8F09,
  CaProfileName = 0x8F0B,
  DeviceName = 0x8F0F,
  TReq = 0x8F11,
};
using namespace ndnph::ndncert::TT;
} // namespace TT

/** @brief Return '/localhop/32=PION' name prefix. */
inline ndnph::Name
getPionPrefix()
{
  static const uint8_t tlv[]{
    0x08, 0x08, 0x6C, 0x6F, 0x63, 0x61, 0x6C, 0x68, 0x6F, 0x70, // localhop
    0x20, 0x04, 0x50, 0x49, 0x4F, 0x4E,                         // 32=PION
  };
  static const ndnph::Name name(tlv, sizeof(tlv));
  return name;
}

/** @brief Return 'pake' component. */
inline ndnph::Component
getPakeComponent()
{
  static const uint8_t tlv[]{ 0x08, 0x04, 0x70, 0x61, 0x6B, 0x65 };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return 'confirm' component. */
inline ndnph::Component
getConfirmComponent()
{
  static const uint8_t tlv[]{ 0x08, 0x07, 0x63, 0x6F, 0x6E, 0x66, 0x69, 0x72, 0x6D };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return 'credential' component. */
inline ndnph::Component
getCredentialComponent()
{
  static const uint8_t tlv[]{
    0x08, 0x0A, 0x63, 0x72, 0x65, 0x64, 0x65, 0x6E, 0x74, 0x69, 0x61, 0x6C
  };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return '32=pion-authenticator' component. */
inline ndnph::Component
getAuthenticatorComponent()
{
  static const uint8_t tlv[]{ 0x20, 0x12, 0x70, 0x69, 0x6F, 0x6E, 0x2d, 0x61, 0x75, 0x74,
                              0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 0x6f, 0x72 };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return '32=pion-authenticated' component. */
inline ndnph::Component
getAuthenticatedComponent()
{
  static const uint8_t tlv[]{ 0x20, 0x12, 0x70, 0x69, 0x6F, 0x6E, 0x2d, 0x61, 0x75, 0x74,
                              0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 0x65, 0x64 };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

} // namespace pake
} // namespace pion

#endif // PION_PAKE_AN_HPP
