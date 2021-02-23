#include "ndn-onboarding.h"

ndnph::Face& face = ndnph::cli::openUplink();
ndnph::StaticRegion<65536> region;

int
main(int argc, char** argv)
{
  if (argc != 4) {
    fprintf(stderr, "%s KEY-ID CA-PROFILE-FILE DEVICE-NAME\n", argv[0]);
    return 1;
  }

  std::string ak = ndnph::cli::checkKeyChainId(argv[1]);
  auto cert = ndnph::cli::loadCertificate(region, ak + "_cert");
  ndnph::EcPrivateKey signer;
  {
    ndnph::EcPublicKey pub;
    ndnph::cli::loadKey(region, ak + "_key", signer, pub);
    signer.setName(cert.getName());
  }

  ndnph::Data caProfile = region.create<ndnph::Data>();
  {
    std::ifstream caProfileFile(argv[2]);
    if (!caProfile || !ndnph::cli::input(region, caProfile, caProfileFile)) {
      fprintf(stderr, "CA profile error\n");
      return 1;
    }
  }

  ndnob::pake::Authenticator authenticator(ndnob::pake::Authenticator::Options{
    face : face,
    caProfile : caProfile,
    cert : cert,
    signer : signer,
    nc : ndnph::tlv::Value(),
    deviceName : ndnph::Name::parse(region, argv[3]),
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
      lastState = state;
      fprintf(stderr, "state=%d\n", static_cast<int>(state));
    }
  }
}
