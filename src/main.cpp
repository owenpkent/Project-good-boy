#include <Arduino.h>
#include <stdio.h>
#include <cstdio>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#if OTA_ENABLE
#include <ArduinoOTA.h>
#endif

#include <WebServer.h>
#include "stepper_28byj.h"

#include "sys/logging.h"
#include "sys/config.h"

#ifndef LED_PIN
#define LED_PIN 2 // Common default on ESP32 dev boards
#endif

#ifndef STEPPER_IN1_PIN
#define STEPPER_IN1_PIN 14
#endif
#ifndef STEPPER_IN2_PIN
#define STEPPER_IN2_PIN 27
#endif
#ifndef STEPPER_IN3_PIN
#define STEPPER_IN3_PIN 26
#endif
#ifndef STEPPER_IN4_PIN
#define STEPPER_IN4_PIN 25
#endif

#ifndef STEPS_PER_DISPENSE
#define STEPS_PER_DISPENSE 180
#endif
#ifndef STEPPER_STEP_DELAY_US
#define STEPPER_STEP_DELAY_US 1200
#endif

static AppConfig g_cfg;
static const char* TAG = "MAIN";

// Web server and actuator
static WebServer server(80);
static Stepper28BYJ g_stepper(STEPPER_IN1_PIN, STEPPER_IN2_PIN, STEPPER_IN3_PIN, STEPPER_IN4_PIN, STEPPER_STEP_DELAY_US);
static QueueHandle_t g_dispenseQ = nullptr; // queue of int counts

// Blink task
void BlinkTask(void*);
// Housekeeping/telemetry task
void HouseTask(void*);
// Dispense worker task
void DispenseTask(void*);

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

// Overload to allow passing C strings (e.g., from std::string::c_str())
static bool connectWiFi(const char* ssid, const char* pass) {
  return connectWiFi(String(ssid ? ssid : ""), String(pass ? pass : ""));
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
  bool wifiOk = connectWiFi(g_cfg.wifi_ssid.c_str(), g_cfg.wifi_pass.c_str());
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

  // Initialize stepper and worker
  g_stepper.begin();
  g_dispenseQ = xQueueCreate(4, sizeof(int));
  xTaskCreatePinnedToCore(DispenseTask, "dispense", 4096, nullptr, 1, nullptr, 0);

  // Minimal REST API
  server.on("/api/status", HTTP_GET, []() {
    String body = "{";
    body += "\"deviceName\":\"";
    body += g_cfg.device_name.c_str();
    body += "\",";
    body += "\"wifi\":{\"connected\":"; body += WiFi.isConnected() ? "true" : "false"; body += ",\"ssid\":\""; body += WiFi.SSID(); body += "\",\"rssi\":"; body += String(WiFi.RSSI()); body += "},";
    body += "\"battery\":{\"percent\":100,\"voltage\":4.10},"; // placeholder
    body += "\"firmware\":\"" FW_VERSION "\"";
    body += "}";
    server.send(200, "application/json", body);
  });

  server.on("/api/dispense", HTTP_POST, []() {
    int count = 1;
    String body = server.arg("plain");
    int idx = body.indexOf("\"count\"");
    if (idx >= 0) {
      int colon = body.indexOf(":", idx);
      if (colon >= 0) {
        String num = body.substring(colon + 1);
        // strip non-digit characters except minus
        String digits;
        for (size_t i = 0; i < num.length(); ++i) {
          char c = num[i];
          if ((c >= '0' && c <= '9') || c == '-') digits += c;
          else if (digits.length() > 0) break;
        }
        long tmp = digits.toInt();
        if (tmp > 0 && tmp <= 10) count = (int)tmp;
      }
    }
    // enqueue
    if (g_dispenseQ) xQueueSend(g_dispenseQ, &count, 0);
    String resp = String("{\"ok\":true,\"enqueued\":true,\"count\":") + count + "}";
    server.send(200, "application/json", resp);
  });

  server.begin();
  LOGI(TAG, "HTTP server started on port 80");

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
  server.handleClient();
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

void DispenseTask(void*) {
  const char* tag = "DSP";
  for (;;) {
    int count = 0;
    if (g_dispenseQ && xQueueReceive(g_dispenseQ, &count, portMAX_DELAY) == pdTRUE) {
      LOGI(tag, "Dispense request: %d", count);
      for (int i = 0; i < count; ++i) {
        g_stepper.step(STEPS_PER_DISPENSE);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      LOGI(tag, "Dispense complete");
    }
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
