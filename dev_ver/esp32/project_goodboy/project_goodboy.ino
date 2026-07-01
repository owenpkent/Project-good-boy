/*
  Sources:

  FreeRTOS API and Examples - freertos.org

  Espressif Systems docs.espressif.com

  ChatGPT - OpenAI.com

  Rui Santos - RandomNerdTutorials.com

  Last Minute Engineers - https://lastminuteengineers.com/a4988-stepper-motor-driver-arduino-tutorial/

  Adafruit Learn - https://learn.adafruit.com/ir-breakbeam-sensors/arduino

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"
#include <ESPmDNS.h>
#include <driver/adc.h>
#include <Wire.h>

#define TCA9548_ADDR 0x70
#define STM32_ADDR 0x42
#define CMD_STOP       0
#define CMD_DISPENSE   1
#define MIN_SPEED 100
#define MAX_SPEED 1000
#define R1 100000.0f   // 100k
#define R2 33000.0f    // 33k
#define ADC_MAX 4095.0f
#define ADC_REF 3.3f  
#define ACC 800
#define STM32_CHANNEL 1
#define I2C_SDA 17
#define I2C_SCL 16

const char *AP_SSID = "GoodBoy";
const char *AP_PASS = "buddythedog";

const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";

AsyncWebServer server(80);

const int batt = 5;
volatile float g_battVoltage = 0.0f;
volatile bool stopRequested;
String ssid, pass;

TaskHandle_t TaskWiFiHandle = NULL;

bool connectWiFi();
void startAP();
void startWebServer();
void initLittleFS();
void setupADC();
void selectChannel();
void sendCommand(uint8_t cmd, uint16_t speed);
void saveCredentials(fs::FS &fs, const char *path, const char *message);
String loadCredentials(fs::FS &fs, const char *path);

/******
* RebootTask - Free RTOS Task
* Handles restarting ESP32 when needed
*******/
void RebootTask(void *param) {
  uint32_t ms = (uint32_t)(uintptr_t)param;
  vTaskDelay(ms / portTICK_PERIOD_MS);
  Serial.println("Restarting now...");
  ESP.restart();
  vTaskDelete(NULL);
}

/******
* BatteryTask - Free RTOS Task
* Reads battery voltage using a voltage divider
*******/
void BatteryTask(void *parameter) {
  for (;;) {
    int adc_raw = analogRead(batt);
    float v_adc = (adc_raw / ADC_MAX) * ADC_REF;
    float v_batt = v_adc * ((R1 + R2) / R2);  // Compensate voltage divider
    g_battVoltage = v_batt;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/****************
* WifiTask - FreeRTOS task
* Attempts to connect to a network.
* If that fails, sets up an access point.
* Opens connection to controls webpage.
*****************/
void WiFiTask(void *parameter) {
  Serial.println("WiFi Task started");
  
  ssid = loadCredentials(LittleFS, ssidPath);
  pass = loadCredentials(LittleFS, passPath);
  
  Serial.print("Loaded SSID: ");
  Serial.println(ssid.isEmpty() ? "(empty)" : ssid);
  
  if (connectWiFi()) {
    Serial.println("Connected to WiFi");
    if (MDNS.begin("goodboy")) {
     Serial.println("mDNS started: http://goodboy.local");
    } 
    else {
      Serial.println("mDNS failed");
    }
    } 
  else {
    Serial.println("Starting AP mode");
    startAP();
  }

  Serial.println("Starting web server");
  startWebServer();
  Serial.println("Web server started");
  
  vTaskSuspend(NULL);  
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  selectChannel();
  //Serial.print("Reset reason: ");
  //Serial.println(esp_reset_reason());
  initLittleFS();
  setupADC();
  xTaskCreatePinnedToCore(
    WiFiTask,
    "WiFiTask",
    4096,
    NULL,
    1,
    &TaskWiFiHandle,
    0);

  xTaskCreatePinnedToCore(
    BatteryTask,
    "BatteryTask",
    5120,
    NULL,
    1,
    NULL,
    1
  );
}

/***********
* connectWifi - bool func
* Connects to saved credentials.
* Returns:
* false - starts AP (should open DNS automatically)
* true - connection successful, visit at http://goodboy.local
************/
bool connectWiFi() {
  WiFi.disconnect(true);
  if (ssid.isEmpty() || pass.isEmpty()) {
    return false;
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Device IP: ");
    Serial.println(WiFi.localIP().toString());
    return true;
  }
  return false;
}

/**********
* startAP - void funct
* Starts access point at: http://192.168.4.1
***********/
void startAP() {
  Serial.println("Setting WiFi mode to AP");
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  Serial.println("Configuring AP");
  WiFi.softAPConfig(apIP, gateway, subnet);
  Serial.print("Starting AP: ");
  Serial.println(AP_SSID);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("Starting DNS server");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("AP setup complete");
}

/*******
* startWebServer - void funct
* Starts webpage, controls:
* stepper (dog treat dispenser)
* saving wifi credentials
********/
void startWebServer() {
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "404 Not found");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", g_battVoltage);
    request->send(200, "application/json", String("{\"voltage\":") + buf + "}");
  });

  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
    int spd = MIN_SPEED;
    if (request->hasParam("speed")) {
      spd = constrain(request->getParam("speed")->value().toInt(), MIN_SPEED, MAX_SPEED);
    }
    sendCommand(CMD_DISPENSE, spd);
    Serial.println("Dispense command sent");
    request->send(200, "text/plain", "Stepper command updated");
  });
  
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendCommand(CMD_STOP, 0);
    request->send(200, "text/plain", "Stop Sent");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      const AsyncWebParameter *p = request->getParam(i);
      if (p->isPost()) {
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value();
          saveCredentials(LittleFS, ssidPath, ssid.c_str());
        }
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value();
          saveCredentials(LittleFS, passPath, pass.c_str());
        }
      }
    }
    request->send(200, "text/plain", "Credentials saved, restarting in 5s");
    const uint32_t delayMs = 5000;
    xTaskCreatePinnedToCore(
      RebootTask,
      "rebooter",
      2048,
      (void*)(uintptr_t)delayMs,
      1,
      NULL,
      1);
  });
  
  server.begin();
}

/*****
* initLittleFS - void funct
******/
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
  } 
  else {
    Serial.println("LittleFS mounted.");
  }
}

/****
* saveCredentials - void funct
* Parameters: file, path to save credentials to, credentials to save
* Saves wifi credentials to txt file in littleFS
*****/
void saveCredentials(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open %s for writing\n", path);
    return;
  }
  file.print(message);
  file.close();
  Serial.printf("Saved %s\n", path);
}

/*****
* loadCredentials - String funct
* Parameters: file system, path to nab credentials from
* loads credentials from txt in littleFS
******/
String loadCredentials(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return "";
  }
  String content = file.readStringUntil('\n');
  file.close();
  return content;
}

/************
* setupADC - sets up for analog reading
*************/
void setupADC() {
    analogReadResolution(12);           
    analogSetPinAttenuation(batt, ADC_11db);
}

/************
* Data structure to be sent to stm32
**************/
typedef struct __attribute__((packed))
{
    uint8_t command;
    uint16_t speed;
} StepperCommand;

/************
* sendCommand - void func
* Parameters: turn stepper on/off, stepper speed
* Connects to stm32 channel and sends package
*************/
void sendCommand(uint8_t cmd, uint16_t speed)
{
    StepperCommand packet;

    packet.command = cmd;
    packet.speed = speed;
    selectChannel();
    Wire.beginTransmission(STM32_ADDR);
    
    Wire.write((uint8_t*)&packet, sizeof(packet));
    uint8_t err = Wire.endTransmission();
    if (err != 0){
      Serial.printf("Error: %u\n", err);
    }
}

/************
* selectChannel - void func
* sends initial data to begin transmission
*************/
void selectChannel()
{
    Wire.beginTransmission(TCA9548_ADDR);

    Wire.write(1 << STM32_CHANNEL);

    Wire.endTransmission();
}

void loop() {
}

