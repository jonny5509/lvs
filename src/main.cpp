#include <Arduino.h>
#include "esp_log.h"

#include "bluetooth_service.h"
#include "lovense.h"
#include "muse.h"
#include "serial_dongle.h"

void setup() {
  Serial.begin(115200);

  // Reserve the CH340 COM port for the experimental legacy-dongle protocol.
  // Do this before starting BLE modules, which otherwise write diagnostics to
  // the same UART and corrupt protocol traffic.
  serial_dongle_init();

  bluetooth_service_init();
  lovense_init();
  muse_init();
  muse_start();
  bluetooth_service_start();
}

void loop() {
  serial_dongle_poll();
  delay(1);
}
