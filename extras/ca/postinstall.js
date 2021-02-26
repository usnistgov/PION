const fs = require("graceful-fs");

if (!fs.existsSync(".env")) {
  fs.copyFileSync("sample.env", ".env");
}

if (!fs.existsSync("runtime")) {
  fs.mkdirSync("runtime");
}
