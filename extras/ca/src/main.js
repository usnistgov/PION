import { exitClosers } from "@ndn/cli-common";
import { Server, ServerNopChallenge } from "@ndn/ndncert";
import yargs from "yargs";
import { hideBin } from "yargs/helpers";

import { makePossessionChallenge } from "./challenge.js";
import { aProfile, aSigner, initEnv, repo } from "./env.js";

const args = yargs(hideBin(process.argv))
  .scriptName("pion-ca")
  .option("nop", {
    default: false,
    desc: "enable 'nop' challenge (for issuing authenticator certificate)",
    type: "boolean",
  })
  .parseSync();

await initEnv();

/** @type {import("@ndn/ndncert").ServerChallenge[]} */
const challenges = [];
challenges.push(makePossessionChallenge());
if (args.nop) {
  challenges.push(new ServerNopChallenge());
}

const server = Server.create({
  repo,
  profile: aProfile,
  signer: aSigner,
  challenges,
  issuerId: "pion-ca",
});
exitClosers.push(server);
