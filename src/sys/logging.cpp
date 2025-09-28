#include <Arduino.h>
#include "sys/logging.h"
#include "config.h"

namespace Log {
  static Level g_level = (Level)LOG_LEVEL;

  void begin(unsigned long baud) {
    Serial.begin(baud);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) {
      delay(10);
    }
    setLevel((Level)LOG_LEVEL);
    Serial.printf("\n[%s] Boot. FW %s\n", PROJECT_NAME, FW_VERSION);
  }

  void setLevel(Level l) { g_level = l; }

  Level level() { return g_level; }

  void hexdump(const void* data, size_t len) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
      if ((i % 16) == 0) Serial.printf("\n%04x: ", (unsigned)i);
      Serial.printf("%02X ", p[i]);
    }
    Serial.println();
  }
}
