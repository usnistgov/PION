/** @typedef {import("xo").Options} XoOptions */

/** @type {import("@yoursunny/xo-config")} */
const { js, ts, merge } = require("@yoursunny/xo-config");

/** @type {XoOptions} */
module.exports = {
  ...js,
  overrides: [
    {
      files: [
        "./src/**/*.ts",
      ],
      ...merge(js, ts),
    },
  ],
};
