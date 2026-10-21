# LVS-Gateway (PlatformIO)

ESP32 firmware for LVS-Gateway, based on [IngeniousKink/LVS-Gateway](https://github.com/IngeniousKink/LVS-Gateway).

## Build and flash a classic ESP32

The board detected in the serial log as `ESP32-D0WD-V3` should use the `esp32dev` environment.

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension.
2. Clone or download this repository and open the repository folder (the folder containing `platformio.ini`) in VS Code.
3. Connect the ESP32 with a USB data cable. Close other serial-monitor or flashing apps that may have the port open.
4. In the PlatformIO terminal, run:

```sh
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -e esp32dev
```

The first command builds only. The second builds and uploads; the third opens the serial monitor at 115200 baud. If PlatformIO cannot choose the serial port automatically, specify it explicitly:

```sh
pio run -e esp32dev -t upload --upload-port COM3
pio device monitor -e esp32dev --port COM3
```

Replace `COM3` with the port shown by your OS (for example, `/dev/ttyUSB0` on Linux or `/dev/cu.wchusbserialXXXX` on macOS). On Linux, you may need serial-port permissions.

If upload fails with a connection or sync error, hold **BOOT**, start the upload, and release BOOT when the output says it is connecting. Some boards need a brief press of **EN/RESET** after the upload.

## ESP32-S3

For an actual ESP32-S3 board only, use:

```sh
pio run -e esp32s3
pio run -e esp32s3 -t upload
pio device monitor -e esp32s3
```

## Included build fixes

- Pins NimBLE-Arduino to 1.4.2 for the advertising API used by this code.
- Uses the `esp32dev` PlatformIO target for the classic ESP32-D0WD-V3.
- Keeps the ESP32-S3 target available as a separate environment.
- Builds the firmware and prepares ESP Web Tools artifacts in GitHub Actions.

## Web installer

The web installer is at https://jonny5509.github.io/lvs/. The installer can only flash the firmware that CI successfully builds and publishes; successful compilation alone does not prove that every connected sensor or peripheral is wired/configured correctly.
