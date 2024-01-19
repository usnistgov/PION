#include "app.hpp"

namespace pion_device_app {

static constexpr size_t passwordL = 8;
static uint8_t passwordV[passwordL + 1];

void
doMakePassword() {
  GotoState gotoState;
  if (!ndnph::port::RandomSource::generate(passwordV, passwordL)) {
    return;
  }
  passwordV[passwordL] = 0;

  for (size_t i = 0; i < passwordL; ++i) {
    passwordV[i] =
      ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")[passwordV[i] % 0x40];
  }

  NDNPH_LOG_LINE("pion.O.password", "%s", reinterpret_cast<const char*>(passwordV));
  gotoState(State::WaitDirectConnect);
}

ndnph::tlv::Value
getPassword() {
  return ndnph::tlv::Value(passwordV, passwordL);
}

} // namespace pion_device_app
