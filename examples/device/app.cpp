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
      doDirectConnect();
      break;
    }
    case State::WaitPake: {
      waitPake();
      break;
    }
    case State::WaitDirectDisconnect: {
      doDirectDisconnect();
      break;
    }
    case State::WaitInfraConnect: {
      doInfraConnect();
      break;
    }
    case State::WaitNdncert: {
      waitNdncert();
      break;
    }
    case State::Success: {
      Serial.printf("%lu [ndnob.O.cert] ", millis());
      Serial.println(getDeviceCert().getName());
      // fallthrough
    }
    case State::Failure: {
      deletePakeDevice();
      deleteNdncert();
      state = State::Final;
      break;
    }
    case State::Final: {
      break;
    }
  }
}

} // namespace ndnob_device_app
