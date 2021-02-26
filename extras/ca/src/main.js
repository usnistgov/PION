import { Server, ServerNopChallenge } from "@ndn/ndncert";
import yargs from "yargs";
import { hideBin } from "yargs/helpers";

import { makePossessionChallenge } from "./challenge.js";
import { aProfile, aSigner, initEnv, repo } from "./env.js";

(async () => {
const args = (/** @type {import("yargs").Argv} */(/** @type {unknown} */(yargs())))
  .scriptName("ndn-onboarding-ca")
  .option("nop", {
    default: false,
    desc: "enable 'nop' challenge (for issuing authenticator certificate)",
    type: "boolean",
  })
  .parse(hideBin(process.argv));

await initEnv();

/** @type {import("@ndn/ndncert").ServerChallenge[]} */
const challenges = [];
challenges.push(makePossessionChallenge());
if (args.nop) {
  challenges.push(new ServerNopChallenge());
}

Server.create({
  repo,
  profile: aProfile,
  signer: aSigner,
  challenges,
  issuerId: "ndnob-ca",
});
})();
