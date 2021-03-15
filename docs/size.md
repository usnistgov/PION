# Firmware Size Measurement

`size.sh` is a script to measure ESP32 firmware size.
It works on Windows 10 in Git Bash prompt.
It requires `jq`, installed via Chocolatey.

1. Open the sketch in Arduino IDE.
2. In Arduino Tools menu, select correct "Broad" and "Partition Scheme" parameters.
3. Click "Verify" button on the toolbar.
4. In `$USERPROFILE/AppData/Local`, find an `arduino_build_*` directory with the latest modify timestamp.
5. Run the `size.sh` script inside that directory, and redirect the output to a JSON file.
