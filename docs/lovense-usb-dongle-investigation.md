# Lovense USB dongle compatibility investigation

This is an experimental implementation branch. It does not change the default `main` branch.

## Hardware constraint

The target board uses an ESP32-D0WD-V3 and a CH340 USB-to-UART bridge. Windows enumerates the CH340 as a serial/COM device. Firmware on this ESP32 cannot change the CH340's USB descriptors or make that chip enumerate as native USB HID.

## Protocol references

Community-reverse-engineered notes describe an older Lovense dongle using serial-over-USB at 115200 baud. Known command names include:

- Text command `DeviceType;` (a reported response is `D:105:<12-hex-digit-MAC>;`)
- JSON commands `{"type":"usb","func":"init"}`, `{"type":"usb","func":"search"}`, and `{"type":"usb","func":"stopSearch"}`
- JSON toy commands `{"type":"toy","func":"search"}`, `{"type":"toy","func":"stopSearch"}`, `{"type":"toy","func":"statuss"}`, and `{"type":"toy","func":"command","id":"…","cmd":"Vibrate:20;" }`

Sources:
- https://github.com/buttplugio/stpihkal/issues/35
- https://www.slideshare.net/slideshow/def-con-27-amea-adventures-in-smart-buttplug-penetration-testing/231854636

These sources do not fully specify the JSON response/event envelopes or prove that current Lovense Connect versions accept a generic CH340 COM port. Newer dongle variants use USB HID, which this hardware cannot impersonate directly.

## What this branch adds

- `src/serial_dongle.h` and `src/serial_dongle.cpp`: line-framed serial parser at 115200 baud for the documented text commands and the known JSON command names.
- `platformio.ini`: ArduinoJson dependency for bounded JSON parsing.
- `src/main.cpp`: starts the experimental serial protocol handler and polls it from the main loop.
- The serial handler silences ESP-IDF logs so diagnostic output does not mix with protocol bytes.

Text `DeviceType;`, `Battery;`, `Status:1;`, `Vibrate:n;`, and `PowerOff;` are handled. Vibration commands map to the existing `muse_set_intensity()` advertising implementation. The JSON response envelopes are **provisional guesses**, not verified Lovense-compatible responses. Search currently does not discover a real external BLE toy and deliberately does not fabricate a discovery event.

## Important limitations and next test

1. Build the `esp32dev` PlatformIO environment and flash only the experimental branch.
2. Open the CH340 COM port at 115200 baud, 8-N-1. Send `DeviceType;\r\n` and confirm a `D:105:…;` response.
3. Capture genuine legacy dongle traffic for `usb/init`, `usb/search`, toy discovery, status and command responses. Replace the provisional JSON replies with captured response formats.
4. Test whether the exact Lovense Connect for Windows version accepts a generic COM port. The firmware cannot change the CH340 USB identity.
5. Implement a BLE-central driver only after identifying the actual non-Lovense toy protocol. The current `muse.cpp` broadcasts manufacturer-data packets; it does not connect to the toy.

No successful PlatformIO build or Windows Lovense Connect interoperability test has been performed in this environment. Treat this as an experimental starting point, not a working dongle replacement.
