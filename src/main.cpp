#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#if OTA_ENABLE
#include <ArduinoOTA.h>
#endif

#include "config.h"
#include "sys/logging.h"
#include "sys/config.h"

#ifndef LED_PIN
#define LED_PIN 2 // Common default on ESP32 dev boards
#endif

static AppConfig g_cfg;
static const char* TAG = "MAIN";

// Blink task
void BlinkTask(void*);
// Housekeeping/telemetry task
void HouseTask(void*);

static bool connectWiFi(const String& ssid, const String& pass) {
  if (ssid.isEmpty()) {
    LOGW(TAG, "Wi-Fi SSID empty. Skipping STA connect.");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());
  LOGI(TAG, "Wi-Fi connecting to '%s'...", ssid.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    LOGI(TAG, "Wi-Fi connected. IP=%s RSSI=%d", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }

  LOGE(TAG, "Wi-Fi connect failed.");
  return false;
}

static bool syncTime() {
  configTime(0, 0, NTP_SERVER);
  LOGI(TAG, "NTP sync using %s", NTP_SERVER);
  for (int i = 0; i < 20; ++i) {
    time_t now = time(nullptr);
    if (now > 1600000000) { // sanity: > 2020-09-13
      LOGI(TAG, "Time synced: %ld", now);
      return true;
    }
    delay(500);
  }
  LOGW(TAG, "Time sync timed out.");
  return false;
}

void setup() {
  Log::begin(115200);
  Log::setLevel((Log::Level)LOG_LEVEL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Identify device
  g_cfg.begin();
  g_cfg.load();
  LOGI(TAG, "Device: %s", g_cfg.device_name.c_str());

  // Wi-Fi
  bool wifiOk = connectWiFi(g_cfg.wifi_ssid, g_cfg.wifi_pass);
  if (!wifiOk && String(WIFI_SSID).length() > 0) {
    // Try compile-time credentials as fallback if NVS empty
    wifiOk = connectWiFi(String(WIFI_SSID), String(WIFI_PASS));
  }

#if OTA_ENABLE
  if (wifiOk) {
    ArduinoOTA.setHostname(g_cfg.device_name.c_str());
    ArduinoOTA.onStart([]() { LOGI("OTA", "Start"); });
    ArduinoOTA.onEnd([]() { LOGI("OTA", "End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
      LOGE("OTA", "Error %u", error);
    });
    ArduinoOTA.begin();
    LOGI(TAG, "OTA ready. Hostname=%s", g_cfg.device_name.c_str());
  }
#endif

  if (wifiOk) {
    syncTime();
  }

  // Start tasks
  xTaskCreatePinnedToCore(BlinkTask, "blink", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(HouseTask, "house", 4096, nullptr, 1, nullptr, 0);
}

void loop() {
#if OTA_ENABLE
  if (WiFi.isConnected()) {
    ArduinoOTA.handle();
  }
#endif
  // Simple reconnect tick
  static unsigned long lastAttempt = 0;
  if (!WiFi.isConnected() && millis() - lastAttempt > 10000 && (g_cfg.wifi_ssid.length() || String(WIFI_SSID).length())) {
    lastAttempt = millis();
    LOGW(TAG, "Wi-Fi lost. Reconnecting...");
    if (g_cfg.wifi_ssid.length()) WiFi.begin(g_cfg.wifi_ssid.c_str(), g_cfg.wifi_pass.c_str());
    else WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  delay(20);
}

void BlinkTask(void*) {
  const TickType_t fast = pdMS_TO_TICKS(250);
  const TickType_t slow = pdMS_TO_TICKS(750);
  for (;;) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    vTaskDelay(WiFi.isConnected() ? fast : slow);
  }
}

void HouseTask(void*) {
  for (;;) {
    if (WiFi.isConnected()) {
      LOGD(TAG, "Heap=%u RSSI=%d", ESP.getFreeHeap(), WiFi.RSSI());
    } else {
      LOGD(TAG, "Heap=%u (Wi-Fi disconnected)", ESP.getFreeHeap());
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
