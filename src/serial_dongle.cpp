#include "serial_dongle.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include <esp_mac.h>

#include "muse.h"

namespace {
constexpr size_t kMaxLineLength = 384;
char lineBuffer[kMaxLineLength];
size_t lineLength = 0;
bool overflowed = false;

String macString() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char value[13];
  snprintf(value, sizeof(value), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(value);
}

void writeLine(const String& line) {
  Serial.print(line);
  Serial.print("\r\n");
}

void handleTextCommand(const String& line) {
  if (line == "DeviceType;") {
    // Community notes report this response for an older serial dongle.
    // Identity/response behavior still needs validation against a real dongle.
    writeLine("D:105:" + macString() + ";");
    return;
  }
  if (line == "Battery;") {
    writeLine("100;");
    return;
  }
  if (line == "Status:1;") {
    writeLine("2;");
    return;
  }
  if (line.startsWith("Vibrate:")) {
    const int end = line.indexOf(';');
    if (end < 0) return;
    const int level = constrain(line.substring(8, end).toInt(), 0, 20);
    muse_set_intensity(static_cast<float>(level) / 20.0f);
    writeLine("OK;");
    return;
  }
  if (line == "PowerOff;") {
    muse_set_intensity(0.0f);
    writeLine("OK;");
  }
}

void handleJsonCommand(const String& line) {
  StaticJsonDocument<384> doc;
  if (deserializeJson(doc, line)) return;

  const char* type = doc["type"] | "";
  const char* func = doc["func"] | "";

  // The command names are documented by community reverse engineering.
  // These JSON response envelopes are provisional; compare with a genuine
  // adapter capture before expecting Lovense Connect to accept this firmware.
  if (strcmp(type, "usb") == 0 && strcmp(func, "init") == 0) {
    writeLine("{\"type\":\"usb\",\"func\":\"init\",\"result\":true}");
    return;
  }

  if (strcmp(type, "usb") == 0 &&
      (strcmp(func, "search") == 0 || strcmp(func, "stopSearch") == 0)) {
    // This firmware advertises manufacturer data; it does not scan/connect to
    // an external BLE toy, so it deliberately does not fabricate discovery.
    writeLine(String("{\"type\":\"usb\",\"func\":\"") + func +
              "\",\"result\":true}");
    return;
  }

  if (strcmp(type, "toy") == 0 &&
      (strcmp(func, "search") == 0 || strcmp(func, "stopSearch") == 0 ||
       strcmp(func, "statuss") == 0)) {
    writeLine(String("{\"type\":\"toy\",\"func\":\"") + func +
              "\",\"result\":true}");
    return;
  }

  if (strcmp(type, "toy") == 0 && strcmp(func, "command") == 0) {
    const String cmd = String(doc["cmd"] | "");
    if (cmd.startsWith("Vibrate:")) {
      const int end = cmd.indexOf(';');
      const String value = end >= 0 ? cmd.substring(8, end) : cmd.substring(8);
      const int level = constrain(value.toInt(), 0, 20);
      muse_set_intensity(static_cast<float>(level) / 20.0f);
    } else if (cmd == "PowerOff;" || cmd == "Stop;") {
      muse_set_intensity(0.0f);
    }
    writeLine("{\"type\":\"toy\",\"func\":\"command\",\"result\":true}");
  }
}
}  // namespace

void serial_dongle_init() {
  // The COM port is reserved for protocol bytes. Silence ESP-IDF logs to keep
  // diagnostics from corrupting the serial command stream.
  esp_log_level_set("*", ESP_LOG_NONE);
  lineLength = 0;
  overflowed = false;
}

void serial_dongle_poll() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      if (overflowed) {
        overflowed = false;
        lineLength = 0;
        continue;
      }
      if (lineLength == 0) continue;

      lineBuffer[lineLength] = '\0';
      const String line(lineBuffer);
      lineLength = 0;

      if (line == "DeviceType;" || line == "Battery;" ||
          line == "Status:1;" || line.startsWith("Vibrate:") ||
          line == "PowerOff;") {
        handleTextCommand(line);
      } else if (line.startsWith("{")) {
        handleJsonCommand(line);
      }
      continue;
    }

    if (lineLength + 1 >= kMaxLineLength) {
      overflowed = true;
      lineLength = 0;
      continue;
    }
    lineBuffer[lineLength++] = c;
  }
}
