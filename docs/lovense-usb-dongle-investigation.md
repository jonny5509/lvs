# Lovense-compatible BLE and USB dongle investigation

This is an experimental implementation branch. It does not change the default `main` branch.

## Target and architecture

The goal is to let a phone connect to the ESP32 as a Lovense-compatible BLE gateway and have it broadcast vibration commands to compatible MuSE/LoveSpouse toys. Windows support is a separate legacy serial-dongle emulation path and is not confirmed.

The board uses an ESP32-D0WD-V3 and a CH340 USB-to-UART bridge. Windows enumerates the CH340 as a serial/COM device. Firmware on this ESP32 cannot change the CH340's USB descriptors or make that chip enumerate as native USB HID. Older Lovense dongles used serial, while newer adapters use HID; the serial protocol still requires exact compatible replies.

## BLE profile implemented

`src/lovense.cpp` uses the commonly documented legacy Lovense GATT layout:

- Service: `0000fff0-0000-1000-8000-00805f9b34fb`
- FFF1: response/notification characteristic
- FFF2: command/write characteristic

Commands such as `DeviceType;`, `Battery;`, `Status:1;`, `Vibrate:n;`, and `PowerOff;` are handled. The implementation advertises as `LVS-Gateway01`, but a matching GATT profile alone does not guarantee official Lovense Connect recognition. Lovense device generations can use different identity and discovery behavior.

On phone connection, firmware attempts to restart advertising so the toy-control manufacturer data remains available while the phone is connected. The ESP32 has one BLE radio; behavior when advertising and maintaining the GATT connection simultaneously must be verified on the actual board.

## MuSE / LoveSpouse broadcasts

`src/muse.cpp` broadcasts the manufacturer-data prefix `6D B6 43 CE 97 FE 42 7C` and four all-channel commands: stop and three intensity levels. The intensity mapping clamps values before quantization so 100% maps to the highest supported speed rather than a channel-specific payload.

`src/bluetooth_service.cpp` configures a 250 ms advertising interval. The payload is refreshed when each control command arrives, including repeated commands at the same intensity.

This is broadcast-based control, not a BLE-central connection to a real toy. Compatible toys within radio range may receive the same broadcast. Exact compatibility depends on toy firmware and manufacturer-data prefix. Test with the toy close to the board and be ready to disconnect power if it does not stop as expected.

## Experimental legacy USB serial handler

`src/serial_dongle.cpp` implements a line-framed handler at 115200 baud for a subset of legacy serial dongle commands:

- `DeviceType;`
- `Battery;`
- `Status:1;`
- JSON commands such as `usb/init`, `usb/search`, `toy/search`, and `toy/command`

The JSON response envelopes remain provisional. The current search responses do not enumerate a verified toy record, so this handler must not be considered a working Windows Lovense Connect dongle yet. A genuine adapter capture or a test with the target Windows app is needed to confirm exact responses and discovery behavior.

## Build and flash the existing board

The GitHub Actions workflow builds the `esp32dev` environment and uploads a `lvs-esp32dev-build` artifact containing generated `.bin` files.

1. Open [the experimental branch](https://github.com/jonny5509/lvs/tree/experiment/lovense-serial-dongle) in a browser or clone the repository.
2. Install Visual Studio Code and the PlatformIO IDE extension.
3. In VS Code, use **PlatformIO: Pick Project Environment** and select `esp32dev`.
4. Connect the board by USB. Close Serial Monitor or any other program holding its COM port.
5. Use **PlatformIO: Upload** to build and flash the firmware. If the upload cannot connect, hold the board's BOOT button while upload starts, then release it when writing begins.
6. Do not flash the `esp32s3` environment; this board is the original ESP32 target.

Alternatively, open the repository folder in a terminal with PlatformIO installed and run:

```sh
pio run -e esp32dev -t upload
```

## Test on the phone first

1. Install a BLE GATT inspection app such as nRF Connect for Mobile. This is only a diagnostic tool, not proof of Lovense Connect compatibility.
2. Scan for `LVS-Gateway01` and connect.
3. Inspect the `FFF0` service. Confirm `FFF1` is readable/notifiable and `FFF2` is writable.
4. Enable notifications on `FFF1`, then write the UTF-8 text `DeviceType;` to `FFF2`. Record the response notification.
5. Write `Vibrate:20;` to `FFF2`. Check whether the intended LoveSpouse/MuSE toy reacts. Then write `Vibrate:0;` and confirm it stops.
6. Disconnect the diagnostic app. Open Lovense Connect and test whether it discovers/accepts the ESP32. Record the app version and exact behavior. The GATT test succeeding does not guarantee Connect will accept it.

## Test Windows serial separately

1. Connect the board and find its CH340 COM port in Windows Device Manager.
2. Close Arduino/PlatformIO monitors before launching Lovense software.
3. Test the exact Lovense Windows app version and record whether it attempts to open the COM port. The serial handler uses 115200 baud.
4. Do not interpret the board appearing as a COM port as proof that Lovense Connect supports it. If the app expects the official USB Bluetooth Adapter/HID, this firmware cannot change the CH340 into HID.

## Known limits / feedback needed

- No physical board, phone, or toy test has been performed by the maintainer of these changes.
- Phone-side official Lovense Connect recognition is unverified.
- The Windows serial JSON response envelopes are provisional and toy discovery is not implemented.
- The toy path broadcasts fixed manufacturer-data patterns; it does not scan/connect to an external toy as a BLE central.
- If a test fails, please report: which platform, app name/version, whether `LVS-Gateway01` appears, whether FFF0/FFF1/FFF2 appear, and the exact behavior when sending `Vibrate:20;`.

Treat this as an experimental test build, not a verified replacement for an official Lovense dongle.
