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

/** @brief Return '/localhop/32=pion' name prefix. */
inline ndnph::Name
getPionPrefix()
{
  static const uint8_t tlv[]{
    0x08, 0x08, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 'p', 0x20, 0x04, 'p', 'i', 'o', 'n',
  };
  static const ndnph::Name name(tlv, sizeof(tlv));
  return name;
}

/** @brief Return 'pake' component. */
inline ndnph::Component
getPakeComponent()
{
  static const uint8_t tlv[]{ 0x08, 0x04, 'p', 'a', 'k', 'e' };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return 'confirm' component. */
inline ndnph::Component
getConfirmComponent()
{
  static const uint8_t tlv[]{ 0x08, 0x07, 'c', 'o', 'n', 'f', 'i', 'r', 'm' };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return 'credential' component. */
inline ndnph::Component
getCredentialComponent()
{
  static const uint8_t tlv[]{ 0x08, 0x0A, 'c', 'r', 'e', 'd', 'e', 'n', 't', 'i', 'a', 'l' };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return '32=pion-authenticator' component. */
inline ndnph::Component
getAuthenticatorComponent()
{
  static const uint8_t tlv[]{ 0x20, 0x12, 'p', 'i', 'o', 'n', '-', 'a', 'u', 't',
                              'h',  'e',  'n', 't', 'i', 'c', 'a', 't', 'o', 'r' };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

/** @brief Return '32=pion-authenticated' component. */
inline ndnph::Component
getAuthenticatedComponent()
{
  static const uint8_t tlv[]{ 0x20, 0x12, 'p', 'i', 'o', 'n', '-', 'a', 'u', 't',
                              'h',  'e',  'n', 't', 'i', 'c', 'a', 't', 'e', 'd' };
  static const ndnph::Component comp = ndnph::Component::constant(tlv, sizeof(tlv));
  return comp;
}

} // namespace pake
} // namespace pion

#endif // PION_PAKE_AN_HPP
