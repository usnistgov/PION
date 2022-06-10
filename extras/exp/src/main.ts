import * as ndjson from "ndjson";
import { hideBin } from "yargs/helpers";
import yargs from "yargs/yargs";

import { Run } from "./run";

const output = ndjson.stringify();
output.pipe(process.stdout);

const args = yargs(hideBin(process.argv))
  .scriptName("pion-exp")
  .option("count", {
    default: 1,
    desc: "repeat experiment N times",
    type: "number",
  })
  .parseSync();

for (let i = 0; i < args.count; ++i) {
  const run = new Run();
  const result = await run.run();
  output.write(result);
}
