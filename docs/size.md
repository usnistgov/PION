# Firmware Size Measurement

[firmware-sizes.sh](../extras/firmware-sizes.sh) measures firmware size under different combinations.

1. Install Arduino IDE and prepare the sketch as in [installation guide](INSTALL.md).

2. Install Chocolatey packages:

   ```powershell
   choco install arduino-cli git jq
   ```

3. Run the script in Git Bash:

   ```bash
   extras/firmware-sizes.sh > extras/firmware-sizes.ndjson
   ```
