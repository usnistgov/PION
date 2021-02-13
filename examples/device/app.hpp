#ifndef NDNOB_DEVICE_APP_HPP
#define NDNOB_DEVICE_APP_HPP

#include "config.hpp"
#include <WiFi.h>
#include <ndn-onboarding.h>

#define NDNOB_LOG_ERR(fmt, ...) Serial.printf("%lu [ndnob.E] " fmt "\n", millis(), ##__VA_ARGS__)
#define NDNOB_LOG_STATE(kind, value)                                                               \
  do {                                                                                             \
    static int prev = -1;                                                                          \
    if (prev != (int)(value)) {                                                                    \
      Serial.printf("%lu [ndnob.S.%s] %d\n", millis(), kind, (int)(value));                        \
      prev = (int)(value);                                                                         \
    }                                                                                              \
  } while (false)

namespace ndnob_device_app {

enum class State
{
  Idle,
  WaitPassword,
  WaitDirectConnect,
  WaitPake,
  WaitDirectDisconnect,
  WaitInfraConnect,
  WaitNdncert,
  Success,
  Failure,
};

extern State state;

class GotoState
{
public:
  bool operator()(State st)
  {
    state = st;
    m_set = true;
    return true;
  }

  ~GotoState()
  {
    if (!m_set) {
      state = State::Failure;
    }
  }

private:
  bool m_set = false;
};

void
loop();

void
waitDirectConnect();
void
waitPake();
void
waitDirectDisconnect();

} // namespace ndnob_device_app

#endif // NDNOB_DEVICE_APP_HPP
