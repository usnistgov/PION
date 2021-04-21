# Firmware Size Measurement

[firmware-sizes.sh](../extras/firmware-sizes.sh) measures firmware size under different combinations.
It is designed to run on Windows 10 environment.

1. Install Arduino IDE and prepare the sketch as in [installation guide](INSTALL.md).

2. Install Chocolatey packages:

   ```powershell
   choco install arduino-cli git jq
   ```

3. Run the script in Git Bash:

   ```bash
   extras/firmware-sizes.sh > extras/firmware-sizes.ndjson
   ```
