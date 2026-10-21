# Lovense USB dongle compatibility investigation

This is an experimental investigation branch. It intentionally does not change the default firmware or claim that the current board is already compatible with Lovense Connect.

## Hardware constraint

The target board uses an ESP32-D0WD-V3 and a CH340 USB-to-UART bridge. Windows enumerates the CH340 as a serial/COM device. Firmware on this ESP32 cannot change the CH340's USB descriptors or make that chip enumerate as native USB HID.

## What the public protocol notes establish

The older Lovense dongle has a documented community-reverse-engineered serial protocol at 115200 baud, 8 data bits, no parity, 1 stop bit. A DEF CON 27 presentation describes CRLF-terminated commands and a simple initial handshake:

- Send `DeviceType;\r\n`; a documented response format is `D:105:<12-hex-digit-MAC>;\n`.
- JSON commands include `{"type":"usb","func":"init"}`, `{"type":"usb","func":"search"}`, and `{"type":"usb","func":"stopSearch"}`.
- Toy commands include `{"type":"toy","func":"search"}`, `{"type":"toy","func":"stopSearch"}`, `{"type":"toy","func":"statuss"}`, `{"type":"toy","func":"connect","eager":1,"id":"…" }`, and `{"type":"toy","func":"command","id":"…","cmd":"Vibrate:20;" }`.

Sources:
- https://github.com/buttplugio/stpihkal/issues/35
- https://www.slideshare.net/slideshow/def-con-27-amea-adventures-in-smart-buttplug-penetration-testing/231854636

The community notes do not establish that current Lovense Connect versions accept a generic CH340 COM port with non-Lovense USB IDs. The newer dongle variant uses USB HID, which this hardware cannot impersonate directly.

## Existing firmware gap

At the rollback baseline, `src/bluetooth_service.cpp` advertises a BLE peripheral named `LVS-Gateway01`. `src/lovense.cpp` implements a BLE GATT characteristic for toy-style text commands. `src/muse.cpp` changes manufacturer data in BLE advertisements to represent intensity states. These pieces do not yet implement the USB serial dongle protocol or scan/connect to an external BLE toy as a central.

Also, `Serial` is currently used for diagnostic logging. A serial protocol implementation must keep logs off the protocol stream (or put logs behind a compile-time flag), parse framed JSON commands, and emit only protocol-compatible responses on the COM port.

## Safe implementation sequence

1. Test whether the target Lovense Connect version enumerates and opens a generic COM port, or requires the Lovense adapter's USB identity. The firmware cannot change the CH340's USB IDs.
2. Obtain/capture known-good request/response traffic from an older genuine serial Lovense dongle at 115200 baud, including initialization, search results, status events, and command responses. The published command list alone is not a complete specification.
3. Implement a separate, opt-in serial protocol mode on this experimental branch, keeping debug logs off the protocol stream.
4. Implement a BLE-central driver for the actual non-Lovense toy and route serial commands to it. The current BLE peripheral/advertising implementation is not that driver.
5. Test against Windows and the target Connect version before considering any merge.

## Current status

No changes to the default `main` branch. The experimental branch records the verified constraints and protocol leads without disturbing the rollback point. A complete compatible implementation cannot be verified from the currently available command list alone.
