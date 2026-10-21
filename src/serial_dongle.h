#pragma once

// Experimental emulation of the legacy serial-over-USB Lovense dongle protocol.
// This uses the existing CH340 COM port; it cannot emulate USB HID descriptors.
void serial_dongle_init();
void serial_dongle_poll();
