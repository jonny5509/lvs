#include "serial_dongle.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_mac.h>

#include "muse.h"

namespace {
constexpr size_t kMaxLineLength = 384;
char lineBuffer[kMaxLineLength];
size_t lineLength = 0;
bool overflowed = false;
bool initialized = false;
String toyId;

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

String jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == '"' || c == '\\') out += '\\';
    if (c == '\n') { out += "\\n"; continue; }
    if (c == '\r') { out += "\\r"; continue; }
    out += c;
  }
  return out;
}

void handleTextCommand(const String& line) {
  if (line == "DeviceType;") {
    // Legacy serial dongle DeviceType response reported by community protocol notes.
    // The D:105 identity is a compatibility hypothesis and must be checked against
    // a capture from a genuine dongle before claiming Connect compatibility.
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
    int end = line.indexOf(';');
    if (end < 0) return;
    int level = line.substring(8, end).toInt();
    level = constrain(level, 0, 20);
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
  DeserializationError error = deserializeJson(doc, line);
  if (error) return;

  const char* type = doc["type"] | "";
  const char* func = doc["func"] | "";

  // These are the public legacy-dongle command names. Exact response/event
  // envelopes are not fully documented, so this experimental implementation
  // intentionally labels its response shape as provisional in the docs.
  if (strcmp(type, "usb") == 0 && strcmp(func, "init") == 0) {
    initialized = true;
    writeLine("{\"type\":\"usb\",\"func\":\"init\",\"result\":true}");
    return;
  }

  if (strcmp(type, "usb") == 0 &&
      (strcmp(func, "search") == 0 || strcmp(func, "stopSearch") == 0)) {
    // The current firmware advertises manufacturer data; it does not scan and
    // connect to a BLE toy. Do not fabricate a "found toy" event here.
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
    const char* command = doc["cmd"] | "";
    String cmd(command);
    if (cmd.startsWith("Vibrate:")) {
      int end = cmd.indexOf(';');
      String value = end >= 0 ? cmd.substring(8, end) : cmd.substring(8);
      int level = constrain(value.toInt(), 0, 20);
      muse_set_intensity(static_cast<float>(level) / 20.0f);
    } else if (cmd == "PowerOff;" || cmd == "Stop;") {
      muse_set_intensity(0.0f);
    }
    writeLine("{\"type\":\"toy\",\"func\":\"command\",\"result\":true}");
  }
}
}  // namespace

void serial_dongle_init() {
  // Serial.begin(115200) is called in setup(). Suppress ESP-IDF diagnostics so
  // the COM port is reserved for protocol traffic rather than mixed log output.
  esp_log_level_set("*", ESP_LOG_NONE);
  toyId = macString();
  initialized = false;
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
      String line(lineBuffer);
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
