#pragma once
#include <Arduino.h>

namespace Log {
  enum Level { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 };
  void begin(unsigned long baud = 115200);
  void setLevel(Level l);
  Level level();
  void hexdump(const void* data, size_t len);
}

#ifndef LOG_TAG
#define LOG_TAG "APP"
#endif

#define LOG_CAN(lvl) (Log::level() >= (lvl))

#define LOGE(tag, fmt, ...) do { if (LOG_CAN(Log::ERROR)) Serial.printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define LOGW(tag, fmt, ...) do { if (LOG_CAN(Log::WARN))  Serial.printf("[W] %s: " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define LOGI(tag, fmt, ...) do { if (LOG_CAN(Log::INFO))  Serial.printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define LOGD(tag, fmt, ...) do { if (LOG_CAN(Log::DEBUG)) Serial.printf("[D] %s: " fmt "\n", tag, ##__VA_ARGS__); } while(0)
