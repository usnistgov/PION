#include "app.hpp"

namespace ndnob_device_app {

State state = State::Idle;

void
loop()
{
  NDNOB_LOG_STATE("app", state);
  switch (state) {
    case State::Idle: {
      state = State::WaitPassword;
      break;
    }
    case State::WaitPassword: {
      state = State::WaitDirectConnect;
      break;
    }
    case State::WaitDirectConnect: {
      waitDirectConnect();
      break;
    }
    case State::WaitPake: {
      waitPake();
      break;
    }
    case State::WaitDirectDisconnect: {
      waitDirectDisconnect();
      break;
    }
    case State::WaitInfraConnect: {
      break;
    }
    case State::WaitNdncert: {
      break;
    }
    case State::Success: {
      break;
    }
    case State::Failure: {
      break;
    }
  }
}

} // namespace ndnob_device_app
