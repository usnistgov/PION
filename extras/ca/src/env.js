import { closeUplinks, openKeyChain, openUplinks } from "@ndn/cli-common";
import { Certificate, createVerifier, generateSigningKey } from "@ndn/keychain";
import { CaProfile } from "@ndn/ndncert";
import { Data, Name } from "@ndn/packet";
import { DataStore, PrefixRegStatic, RepoProducer } from "@ndn/repo";
import { Decoder, Encoder } from "@ndn/tlv";
import strattadbEnvironment from "@strattadb/environment";
import dotenv from "dotenv";
import gracefulfs from "graceful-fs";
import leveldown from "leveldown";

const { promises: fs } = gracefulfs;
const { makeEnv, parsers } = strattadbEnvironment;

dotenv.config();

export const env = makeEnv({
  profile: {
    envVarName: "PION_CA_PROFILE",
    parser: parsers.string,
    required: true,
  },
  repo: {
    envVarName: "PION_REPO",
    parser: parsers.string,
    required: true,
  },
  networkPrefix: {
    envVarName: "PION_NETWORK_PREFIX",
    parser: parsers.string,
    required: true,
  },
});

export const networkPrefix = new Name(env.networkPrefix);

export const repo = new DataStore(leveldown(env.repo));

/** @type {RepoProducer} */
let repoProducer;

export const keyChain = openKeyChain();

/** @type {import("@ndn/ndncert").CaProfile} */
export let aProfile;

/** @type {import("@ndn/packet").Signer} */
export let aSigner;

/** @type {import("@ndn/packet").Verifier} */
export let aVerifier;

export async function initEnv() {
  await openUplinks();
  repoProducer = RepoProducer.create(repo, {
    reg: PrefixRegStatic(networkPrefix),
  });

  try {
    const profileData = new Decoder(await fs.readFile(env.profile)).decode(Data);
    aProfile = await CaProfile.fromData(profileData);
    aSigner = await keyChain.getSigner(aProfile.cert.name);
  } catch {
    const [pvt, pub] = await generateSigningKey(keyChain, networkPrefix);
    const cert = await Certificate.selfSign({
      privateKey: pvt,
      publicKey: pub,
    });
    await keyChain.insertCert(cert);
    aSigner = await keyChain.getSigner(cert.name);
    aProfile = await CaProfile.build({
      prefix: networkPrefix,
      info: "NDN onboarding CA",
      probeKeys: [],
      maxValidityPeriod: 30 * 86400 * 1000,
      cert,
      signer: aSigner,
    });
    await fs.writeFile(env.profile, Encoder.encode(aProfile.data));
  }
  await repo.insert(aProfile.cert.data);
  aVerifier = await createVerifier(aProfile.cert);
}

process.once("beforeExit", async () => {
  repoProducer?.close();
  await repo.close();
  await closeUplinks();
});
