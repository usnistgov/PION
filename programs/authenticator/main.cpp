#include "ndn-onboarding.h"

ndnph::Face& face = ndnph::cli::openUplink();

int
main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr, "%s KEY-ID CA-PROFILE", argv[0]);
    return 1;
  }
  std::string ak = ndnph::cli::checkKeyChainId(argv[1]);

  ndnph::DynamicRegion keyRegion(8192);
  ndnph::Data caProfile = keyRegion.create<ndnph::Data>();
  {
    std::ifstream caProfileFile(argv[2]);
    if (!caProfile || !ndnph::cli::input(keyRegion, caProfile, caProfileFile)) {
      fprintf(stderr, "CA profile error\n");
      return 1;
    }
  }

  auto cert = ndnph::cli::loadCertificate(keyRegion, ak + "_cert");
  ndnph::EcPrivateKey pvt;
  ndnph::EcPublicKey pub;
  ndnph::cli::loadKey(keyRegion, ak + "_key", pvt, pub);

  ndnob::pake::Authenticator authenticator(ndnob::pake::Authenticator::Options{
    face : face,
    caProfile : caProfile,
    cert : cert,
    pvt : pvt,
  });
  if (!authenticator.begin(ndnph::tlv::Value::fromString("password"))) {
    fprintf(stderr, "authenticator.begin error\n");
    return 1;
  }

  auto lastState = ndnob::pake::Authenticator::State::Idle;
  for (;;) {
    ndnph::port::Clock::sleep(1);
    face.loop();

    auto state = authenticator.getState();
    if (state != lastState) {
      fprintf(stderr, "state=%d\n", static_cast<int>(state));
      if (state == ndnob::pake::Authenticator::State::WaitConfirmResponse) {
        fprintf(stderr, "key=");
        for (auto b : authenticator.getKey()) {
          fprintf(stderr, "%02X", b);
        }
        fprintf(stderr, "\n");
      }
      lastState = state;
    }
  }
}
