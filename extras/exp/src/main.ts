import { Run } from "./run";

(async () => {
const run = new Run();
const result = await run.run();
process.stdout.write(JSON.stringify(result));
})();
