# Lovense USB dongle compatibility investigation

This is an experimental investigation branch. It intentionally does not change the default firmware or claim that the current board is already compatible with Lovense Connect.

## Hardware constraint

The target board uses an ESP32-D0WD-V3 and a CH340 USB-to-UART bridge. Windows enumerates the CH340 as a serial/COM device. Firmware on this ESP32 cannot change the CH340's USB descriptors or make that chip enumerate as native USB HID.

## What the public protocol notes establish

The Buttplug/Stpihkal notes for the older Lovense dongle describe a serial-over-USB interface at 115200 baud and JSON commands including:

- `{"type":"usb","func":"init"}`
- `{"type":"usb","func":"search"}`
- `{"type":"usb","func":"stopSearch"}`
- `{"type":"toy","func":"search"}`
- `{"type":"toy","func":"stopSearch"}`
- `{"type":"toy","func":"statuss"}`
- `{"type":"toy","func":"command","id":"…","cmd":"…"}`

Source: https://github.com/buttplugio/stpihkal/issues/35

These notes list commands but do not document the complete response/event format or establish that current Lovense Connect versions accept a generic CH340 COM port. The newer Lovense dongle variant is HID, which this hardware cannot impersonate directly.

## Existing firmware gap

At the rollback baseline, `src/bluetooth_service.cpp` advertises a BLE peripheral named `LVS-Gateway01`. `src/lovense.cpp` implements a BLE GATT characteristic for toy-style text commands. That is not the same interface as the USB dongle protocol.

Also, `Serial` is currently used for diagnostic logging. A serial protocol implementation must keep logs off the protocol stream (or put logs behind a compile-time flag), parse framed JSON commands, and emit only protocol-compatible responses on the COM port.

## Safe implementation sequence

1. Confirm whether the target Lovense Connect build accepts a COM-port dongle with non-Lovense USB IDs. Do not assume that changing the COM port's friendly name is sufficient.
2. Capture known-good request/response traffic from a genuine older serial Lovense dongle, if available, at 115200 baud. Include line endings, timing, initialization events, search results, and toy status events.
3. Implement a separate, opt-in serial protocol mode on an experimental branch, keeping debug logs off the protocol stream.
4. Connect serial search/command events to a real BLE-central toy driver. The current GATT peripheral implementation alone does not provide this bridge.
5. Test against Windows and the target Connect version before considering any merge.

## Current status

No changes to the default `main` branch. The experimental branch is a place to continue the protocol work without disturbing the known rollback point. A complete compatible implementation cannot be verified from the command list alone.
