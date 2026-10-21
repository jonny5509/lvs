# LVS-Gateway (PlatformIO fix)

This repository contains the LVS-Gateway ESP32 firmware, based on [IngeniousKink/LVS-Gateway](https://github.com/IngeniousKink/LVS-Gateway).

## Build and upload on Windows

1. Install VS Code and the PlatformIO IDE extension.
2. Clone this repository and open the folder in VS Code.
3. Connect your ESP32 with a USB data cable.
4. In the PlatformIO terminal, run one of the following commands:

```powershell
pio run -e esp32dev -t upload
```

For an ESP32-S3 board, use:

```powershell
pio run -e esp32s3 -t upload
```

## Fixes included

- Pin NimBLE-Arduino to 1.4.2, which provides the advertising API used by this code.
- Include Arduino.h in lovense.cpp so Serial is declared.
- Fix the PowerOff handler to send a zero-vibration command instead of passing an integer to a string-based function.
- Run Python build scripts explicitly in GitHub Actions so executable file permissions are not required.

Firmware is built from source; generated web installer files are produced by the CI workflow.
