import * as ndjson from "ndjson";
import yargs, { Argv } from "yargs";
import { hideBin } from "yargs/helpers";

import { Run } from "./run";

const output = ndjson.stringify();
output.pipe(process.stdout);

(async () => {
const args = (yargs() as unknown as Argv)
  .scriptName("ndn-onboarding-exp")
  .option("count", {
    default: 1,
    desc: "repeat experiment X times",
    type: "number",
  })
  .parse(hideBin(process.argv));

for (let i = 0; i < args.count; ++i) {
  const run = new Run();
  const result = await run.run();
  output.write(result);
}
})();
