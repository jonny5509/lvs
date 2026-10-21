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

## MuSE / LoveSpouse broadcasts

`src/muse.cpp` broadcasts the documented manufacturer-data prefix `6D B6 43 CE 97 FE 42 7C` and four all-channel commands: stop and three intensity levels. The intensity mapping clamps values before quantization so 100% maps to the highest supported speed rather than a channel-specific payload.

`src/bluetooth_service.cpp` configures a 250 ms advertising interval, matching the community-documented minimum interval for these toy broadcasts. The payload is updated only when the requested intensity changes, rather than stopping and restarting advertising every second. The same BLE radio advertises the app-facing GATT service and the toy-control manufacturer data; the actual phone/app/toy combination still needs a physical test.

This is broadcast-based control, not a BLE-central connection to a real toy. Compatible toys within radio range are expected to receive the same broadcast. Exact compatibility depends on the toy's firmware and manufacturer-data prefix.

## Experimental legacy USB serial handler

`src/serial_dongle.cpp` implements a line-framed handler at 115200 baud for a subset of legacy serial dongle commands:

- `DeviceType;`
- `Battery;`
- `Status:1;`
- JSON commands such as `usb/init`, `usb/search`, `toy/search`, and `toy/command`

The JSON response envelopes remain provisional. The current search responses do not enumerate a verified toy record, so this handler must not be considered a working Windows Lovense Connect dongle yet. A genuine adapter capture or a test with the target Windows app is needed to confirm exact responses and discovery behavior.

## Platform constraints

- **Phone:** the ESP32 can expose a BLE peripheral, but official Lovense Connect discovery/acceptance and toy response need a real test.
- **Windows:** official documentation says Lovense Connect/Remote on Windows expects the Lovense USB Bluetooth Adapter. A CH340 COM device could only work through the older serial-dongle path if the app version accepts that protocol and the firmware returns the expected discovery responses. That is currently unverified.
- **Both platforms:** successful compilation is not end-to-end app compatibility.

## Test plan

1. Confirm the GitHub Actions PlatformIO build succeeds for `esp32dev`.
2. Flash only this experimental branch to the existing board.
3. On a phone, connect to `LVS-Gateway01`, enumerate FFF0/FFF1/FFF2, subscribe to FFF1 notifications, and send `DeviceType;` and `Vibrate:20;` to FFF2.
4. Verify that the exact LoveSpouse and MuSE models respond to the broadcasts.
5. Test official Lovense Connect on the phone and record whether it discovers and accepts the ESP32.
6. Test the exact Windows app version against the CH340 COM port; record the raw serial request/response traffic.
7. Replace guessed serial response envelopes with measured behavior before claiming Windows compatibility.

Treat this as an improved experimental starting point, not a verified replacement for an official Lovense dongle.
