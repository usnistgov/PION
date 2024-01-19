#ifndef PION_DEVICE_APP_HPP
#define PION_DEVICE_APP_HPP

#include "config.hpp"
#include <WiFi.h>
#include <pion.h>

namespace pion_device_app {

enum class State {
  Idle,
  MakePassword,
  WaitDirectConnect,
  WaitPake,
  WaitDirectDisconnect,
  WaitInfraConnect,
  WaitNdncert,
  Success,
  Failure,
  Final,
};

extern State state;

class GotoState {
public:
  bool operator()(State st) {
    state = st;
    m_set = true;
    return true;
  }

  ~GotoState() {
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
doMakePassword();

ndnph::tlv::Value
getPassword();

void
doDirectConnect();

void
waitPake();

void
doDirectDisconnect();

const pion::pake::Device*
getPakeDevice();

void
deletePakeDevice();

void
doInfraConnect();

void
waitNdncert();

void
deleteNdncert();

const ndnph::Data&
getDeviceCert();

const ndnph::PrivateKey&
getDeviceSigner();

void
runPingServer();

} // namespace pion_device_app

#endif // PION_DEVICE_APP_HPP
