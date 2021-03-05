#include "app.hpp"

namespace ndnob_device_app {

State state = State::Idle;
uint32_t minFreeHeap = std::numeric_limits<uint32_t>::max();

void
loop()
{
  {
    uint32_t freeHeap = ESP.getFreeHeap();
    minFreeHeap = std::min(minFreeHeap, freeHeap);
    if (NDNOB_LOG_STATE("app", state)) {
      NDNOB_LOG_MSG("H.free-prev-state", "%u\n", minFreeHeap);
      minFreeHeap = freeHeap;
    }
  }

  switch (state) {
    case State::Idle: {
      state = State::MakePassword;
      break;
    }
    case State::MakePassword: {
      doMakePassword();
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
      NDNOB_LOG_MSG("O.cert", "");
      Serial.println(getDeviceCert().getName());
      // fallthrough
    }
    case State::Failure: {
      deletePakeDevice();
      deleteNdncert();
      NDNOB_LOG_MSG("H.free-final", "%u\n", ESP.getFreeHeap());
      state = State::Final;
      break;
    }
    case State::Final: {
      break;
    }
  }
}

} // namespace ndnob_device_app
