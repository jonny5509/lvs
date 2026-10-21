# Lovense-compatible BLE and USB dongle investigation

This is an experimental implementation branch. It does not change the default `main` branch.

## Target and architecture

The goal is to let Lovense Connect on a phone recognize this ESP32 as a BLE toy-like endpoint and to relay vibration commands into the existing MuSE-compatible BLE advertisement implementation. Windows Lovense Connect compatibility is a separate target and is not confirmed.

The board uses an ESP32-D0WD-V3 and a CH340 USB-to-UART bridge. Windows enumerates the CH340 as a serial/COM device. Firmware on this ESP32 cannot change the CH340's USB descriptors or make that chip enumerate as native USB HID.

## BLE profile implemented

`src/lovense.cpp` now uses the commonly documented legacy Lovense GATT layout:

- Service: `0000fff0-0000-1000-8000-00805f9b34fb`
- RX / response characteristic: `0000fff1-0000-1000-8000-00805f9b34fb` (read + notify)
- TX / command characteristic: `0000fff2-0000-1000-8000-00805f9b34fb` (write + write without response)

Commands such as `DeviceType;`, `Battery;`, `Status:1;`, `Vibrate:n;`, and `PowerOff;` are handled. The implementation advertises as `LVS-Gateway01`, but the correct GATT profile alone does not guarantee the official app will recognize it. Lovense device generations can use different GATT services, device identities, and app-specific discovery behavior.

## MuSE-compatible advertisement control

`src/muse.cpp` broadcasts manufacturer-data patterns for stop and three intensity levels. The intensity mapping was corrected so 100% maps to the highest supported single-channel pattern (index 3), rather than accidentally selecting a dual-channel stop pattern (index 4).

This is a broadcast-based control approach, not a BLE-central connection to a real toy. Whether a particular LoveSpouse or MuSE model accepts these advertisements must be verified against the actual toy model and its protocol. The current firmware does not scan for toys, connect to toys as a BLE central, or independently discover and route to multiple toys.

## Experimental legacy USB serial handler

The branch also contains `src/serial_dongle.cpp`, an experimental line-framed handler at 115200 baud for a small set of community-reported legacy serial dongle commands. The documented command names include:

- `DeviceType;`
- `Battery;`
- `Status:1;`
- JSON commands such as `usb/init`, `usb/search`, `toy/search`, and `toy/command`

The JSON response envelopes are provisional guesses, not verified Lovense-compatible responses. Search responses do not discover actual toys and should not be treated as proof of a working dongle.

## Platform constraints

- **Phone:** can connect to BLE peripherals directly, but official Lovense Connect recognition still needs a real app test.
- **Windows:** Lovense's own documentation says Lovense Remote on Windows requires its Lovense USB Bluetooth Adapter. This board's CH340 is a serial adapter, not that Bluetooth adapter. Serial protocol emulation may work only with software that explicitly supports the legacy COM-port dongle protocol; current Windows app compatibility is unverified.
- **Both platforms:** no successful end-to-end app + toy test has been performed.

## Build and hardware test plan

1. Build the `esp32dev` PlatformIO environment using the GitHub Actions firmware-build workflow.
2. Flash only this experimental branch to the existing ESP32 board.
3. On a phone, scan for `LVS-Gateway01`, connect, enumerate FFF0/FFF1/FFF2, subscribe to FFF1 notifications, and send `DeviceType;` and `Vibrate:20;` to FFF2.
4. Verify the exact LoveSpouse and MuSE models react to the broadcast patterns.
5. Test the official Lovense Connect app separately; record whether it discovers and accepts the ESP32.
6. For Windows, test the exact Lovense Connect version and record whether it supports the CH340 COM port. Do not assume that a successful serial loopback means the app accepts it.
7. Before relying on this for real toys, capture genuine toy/dongle traffic and replace any guessed responses with measured protocol behavior.

Treat this as an improved experimental starting point, not a verified replacement for an official Lovense dongle.
