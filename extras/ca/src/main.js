import { Server, ServerNopChallenge } from "@ndn/ndncert";
import { createRequire } from "node:module";

import { makePossessionChallenge } from "./challenge.js";
import { aProfile, aSigner, initEnv, repo } from "./env.js";

const require = createRequire(import.meta.url);
const yargs = require("yargs/yargs");
const { hideBin } = require("yargs/helpers");

(async () => {
const args = await yargs(hideBin(process.argv))
  .scriptName("pion-ca")
  .option("nop", {
    default: false,
    desc: "enable 'nop' challenge (for issuing authenticator certificate)",
    type: "boolean",
  })
  .parse();

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
  issuerId: "pion-ca",
});
})();
