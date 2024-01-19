#include "packet.hpp"

namespace pion {
namespace pake {

void
EncryptSession::end() {
  ss = ndnph::Component();
  aes.reset();
}

bool
EncryptSession::begin(ndnph::Region& region) {
  uint8_t value[8];
  if (!ndnph::port::RandomSource::generate(value, sizeof(value))) {
    return false;
  }
  ss = ndnph::Component(region, sizeof(value), value);
  return true;
}

bool
EncryptSession::assign(ndnph::Region& region, ndnph::Name name) {
  if (!ss) {
    ss = name.slice(getPionPrefix().size(), getPionPrefix().size() + 1).clone(region)[0];
  }
  return !!ss && getPionPrefix().isPrefixOf(name) && name[getPionPrefix().size()] == ss;
}

ndnph::Name
EncryptSession::makeName(ndnph::Region& region, const ndnph::Component& verb) {
  return getPionPrefix().append(region, ss, verb);
}

ndnph::tlv::Value
EncryptSession::decrypt(ndnph::Region& region, const Encrypted& encrypted) {
  return aes->decrypt(region, encrypted, ss.value(), ss.length());
}

ndnph::Name
computeTempSubjectName(ndnph::Region& region, ndnph::Name authenticatorCertName,
                       ndnph::Name deviceName) {
  ndnph::Name authenticatorName =
    ndnph::certificate::toSubjectName(region, authenticatorCertName, false);
  ndnph::Name head;
  for (size_t i = 0; i < authenticatorName.size(); ++i) {
    if (authenticatorName[i] == getAuthenticatorComponent()) {
      head = authenticatorName.getPrefix(i);
      break;
    }
  }
  if (!head) {
    return ndnph::Name();
  }

  ndnph::Name tail = deviceName.slice(head.size());
  ndnph::Encoder encoder(region);
  encoder.prepend(ndnph::tlv::Value(head.value(), head.length()), getAuthenticatedComponent(),
                  ndnph::tlv::Value(tail.value(), tail.length()));
  encoder.trim();
  return ndnph::Name(encoder.begin(), encoder.size());
}

} // namespace pake
} // namespace pion
