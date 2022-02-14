import { Certificate, CertNaming, createVerifier } from "@ndn/keychain";
import { Keyword } from "@ndn/naming-convention2";
import { ServerPossessionChallenge } from "@ndn/ndncert";
import { Data } from "@ndn/packet";

import { aVerifier, networkPrefix, repo } from "./env.js";

const AuthenticatorKeyword = Keyword.create("onboarding-authenticator");

const AuthenticatedKeyword = Keyword.create("onboarding-authenticated");

/**
 * Parse device temporary certificate name.
 * @param {import("@ndn/packet").Name} name
 */
function parseAuthenticatorCertName(name) {
  const { subjectName } = CertNaming.parseCertName(name);
  const i = subjectName.comps.findIndex((comp) => comp.equals(AuthenticatorKeyword));
  if (i < 0) {
    throw new Error("no authenticator keyword");
  }

  const networkPrefix = subjectName.getPrefix(i);
  return { networkPrefix };
}

/**
 * Parse device temporary certificate name.
 * @param {import("@ndn/packet").Name} name
 */
function parseTempCertName(name) {
  const { subjectName } = CertNaming.parseCertName(name);
  const i = subjectName.comps.findIndex((comp) => comp.equals(AuthenticatedKeyword));
  if (i < 0) {
    throw new Error("no authenticated keyword");
  }

  const networkPrefix = subjectName.getPrefix(i);
  const deviceName = networkPrefix.append(...subjectName.slice(i + 1).comps);
  return { networkPrefix, deviceName };
}

/**
 * Verify device temporary certificate.
 * @param {import("@ndn/packet").Verifier.Verifiable} data
 * @returns {Promise<void>}
 */
async function verifyTempCert(data) {
  if (!(data instanceof Data)) {
    throw new Error("not a Data packet");
  }
  const tParsed = parseTempCertName(data.name);
  const tCert = Certificate.fromData(data);

  const hParsed = parseAuthenticatorCertName(tCert.issuer);
  if (!tParsed.networkPrefix.equals(networkPrefix) || !hParsed.networkPrefix.equals(networkPrefix)) {
    throw new Error("bad network prefix");
  }

  const hCertData = await repo.get(tCert.issuer);
  const hCert = Certificate.fromData(hCertData);
  const hVerifier = await createVerifier(hCert);

  await hVerifier.verify(data);
  await aVerifier.verify(hCertData);
}

/**
 * Check device name assignment policy in relation to device temporary certificate.
 * @param {import("@ndn/packet").Name} newSubjectName
 * @param {import("@ndn/keychain").Certificate} oldCert
 * @returns {Promise<void>}
 */
async function checkAssignmentPolicy(newSubjectName, oldCert) {
  const { deviceName } = parseTempCertName(oldCert.name);
  if (!deviceName.equals(newSubjectName)) {
    throw new Error("bad subject name");
  }
}

/**
 * Create a possession challenge for device certificates.
 * @returns {ServerPossessionChallenge}
 */
export function makePossessionChallenge() {
  return new ServerPossessionChallenge({
    verify: verifyTempCert,
  }, checkAssignmentPolicy);
}
