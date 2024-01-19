#include "pion.h"

static ndnph::Face& face = ndnph::cli::openUplink();
static ndnph::StaticRegion<65536> region;
static std::string profileFilename;
static std::string authenticatorKeySlot;
static ndnph::Name deviceName;
static ndnph::tlv::Value pakePassword;
static ndnph::tlv::Value networkCredential;

static bool
parseArgs(int argc, char** argv) {
  int c;
  while ((c = getopt(argc, argv, "P:i:n:p:N:")) != -1) {
    switch (c) {
      case 'P': {
        profileFilename = optarg;
        break;
      }
      case 'i': {
        authenticatorKeySlot = ndnph::cli::checkKeyChainId(optarg);
        break;
      }
      case 'n': {
        deviceName = ndnph::Name::parse(region, optarg);
        break;
      }
      case 'p': {
        pakePassword = ndnph::tlv::Value::fromString(optarg);
        break;
      }
      case 'N': {
        networkCredential = ndnph::tlv::Value::fromString(optarg);
        break;
      }
    }
  }

  return argc - optind == 0 && !profileFilename.empty() && !authenticatorKeySlot.empty() &&
         !!deviceName && !!pakePassword;
}

int
main(int argc, char** argv) {
  if (!parseArgs(argc, argv)) {
    fprintf(stderr,
            "%s -P CA-PROFILE-FILE -i AK-SLOT -n DEVICE-NAME -p PASSWORD -N NETWORK-CREDENTIAL\n",
            argv[0]);
    return 1;
  }

  auto cert = ndnph::cli::loadCertificate(region, authenticatorKeySlot + "_cert");
  ndnph::EcPrivateKey signer;
  {
    ndnph::EcPublicKey pub;
    ndnph::cli::loadKey(region, authenticatorKeySlot + "_key", signer, pub);
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

  pion::pake::Authenticator authenticator(pion::pake::Authenticator::Options{
    face: face,
    caProfile: caProfile,
    cert: cert,
    signer: signer,
    nc: networkCredential,
    deviceName: deviceName,
  });
  if (!authenticator.begin(pakePassword)) {
    fprintf(stderr, "authenticator.begin error\n");
    return 1;
  }

  for (;;) {
    ndnph::port::Clock::sleep(1);
    face.loop();

    auto st = authenticator.getState();
    PION_LOG_STATE("pake-authenticator", st);
    switch (st) {
      case pion::pake::Authenticator::State::Success:
        return 0;
      case pion::pake::Authenticator::State::Failure:
        return 1;
      default:
        break;
    }
  }
}
