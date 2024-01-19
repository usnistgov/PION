#include "app.hpp"

namespace pion_device_app {

State state = State::Idle;
uint32_t minFreeHeap = std::numeric_limits<uint32_t>::max();

void
loop() {
  {
    uint32_t freeHeap = ESP.getFreeHeap();
    minFreeHeap = std::min(minFreeHeap, freeHeap);
    if (PION_LOG_STATE("app", state)) {
      NDNPH_LOG_LINE("pion.H.free-prev-state", "%u", minFreeHeap);
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
      NDNPH_LOG_MSG("pion.O.cert", "");
      Serial.println(getDeviceCert().getName());
      // fallthrough
    }
    case State::Failure: {
      deletePakeDevice();
      deleteNdncert();
      NDNPH_LOG_LINE("pion.H.free-final", "%u", ESP.getFreeHeap());
      state = State::Final;
      break;
    }
    case State::Final: {
      runPingServer();
      break;
    }
  }
}

} // namespace pion_device_app
