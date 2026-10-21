#include <Arduino.h>
#include <cmath>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <muse.h>

static const char* TAG = "muse";

static uint16_t MANUFACTURER_ID = 0xFFF0;

#define MANUFACTURER_DATA_LENGTH 11
#define MANUFACTURER_DATA_PREFIX 0x6D, 0xB6, 0x43, 0xCE, 0x97, 0xFE, 0x42, 0x7C

static uint8_t _intensity_value = 0;
static bool _stopping = false;

uint8_t manufacturerDataList[][MANUFACTURER_DATA_LENGTH] = {
    // Stop all channels
    {MANUFACTURER_DATA_PREFIX, 0xE5, 0x15, 0x7D},
    // Set all channels to speed 1
    {MANUFACTURER_DATA_PREFIX, 0xE4, 0x9C, 0x6C},
    // Set all channels to speed 2
    {MANUFACTURER_DATA_PREFIX, 0xE7, 0x07, 0x5E},
    // Set all channels to speed 3
    {MANUFACTURER_DATA_PREFIX, 0xE6, 0x8E, 0x4F},
    // Stop 1st channel (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xD5, 0x96, 0x4C},
    // Set 1st channel to speed 1 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xD4, 0x1F, 0x5D},
    // Set 1st channel to speed 2 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xD7, 0x84, 0x6F},
    // Set 1st channel to speed 3 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xD6, 0x0D, 0x7E},
    // Stop 2nd channel (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xA5, 0x11, 0x3F},
    // Set 2nd channel to speed 1 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xA4, 0x98, 0x2E},
    // Set 2nd channel to speed 2 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xA7, 0x03, 0x1C},
    // Set 2nd channel to speed 3 (only for toys with 2 channels)
    {MANUFACTURER_DATA_PREFIX, 0xA6, 0x8A, 0x0D},
};

void set_manufacturer_data(uint8_t index) {
  if (index >= 4) {
    index = 0;
  }

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->stop();

  uint8_t *manufacturerData = manufacturerDataList[index];
  pAdvertising->setManufacturerData(
    std::string(reinterpret_cast<char *>(&MANUFACTURER_ID), 2) +
    std::string(reinterpret_cast<char *>(manufacturerData), MANUFACTURER_DATA_LENGTH)
  );

  pAdvertising->start();
}

void muse_advertising_task(void *pvParameters) {
  (void)pvParameters;

  while (!_stopping) {
    set_manufacturer_data(_intensity_value);
    delay(1000);
  }

  // Advertise stop-all briefly before task exit.
  for (uint8_t i = 0; i < 10; i++) {
    set_manufacturer_data(0);
    delay(200);
  }
  vTaskDelete(nullptr);
}

void muse_set_intensity(float intensity_percent) {
  // Only four command patterns exist: stop, speed 1, speed 2, speed 3.
  // Clamp before quantizing so 100% maps to index 3, never the dual-channel
  // command entries at indices 4 and above.
  if (std::isnan(intensity_percent) || intensity_percent <= 0.0f) {
    _intensity_value = 0;
    return;
  }
  if (intensity_percent >= 1.0f) {
    _intensity_value = 3;
    return;
  }
  _intensity_value = static_cast<uint8_t>(lroundf(intensity_percent * 3.0f));
  if (_intensity_value > 3) {
    _intensity_value = 3;
  }
}

void muse_start() {
  _stopping = false;
  xTaskCreatePinnedToCore(muse_advertising_task, "muse_advertising_task", 4096,
                          nullptr, 2, nullptr, 0);
}

void muse_stop() {
  _stopping = true;
}

void muse_init() {
}
