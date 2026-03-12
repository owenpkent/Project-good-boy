/*
  Sources:

  freertos.org

  FreeRTOS API and Examples - Espressif Systems docs.espressif.com

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
#include <DNSServer.h>
#include <ESPmDNS.h>

#define STEPS_PER_REV 200
#define MIN_SPEED 1
#define MAX_SPEED 20
#define STEP_DELAY_BASE 21000
#define DIVISOR 2

const char *AP_SSID = "GoodBoy";
const char *AP_PASS = "buddythedog";

const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";

DNSServer dnsServer;
AsyncWebServer server(80);

const int dir = 5;
const int stepp = 19;
const int breakBeam = 2;
String ssid, pass;
int breakState = 0;
TaskHandle_t TaskWiFiHandle = NULL;
QueueHandle_t stepperQueue = NULL;

typedef struct {
  int speed;
} StepperJob;

void WiFiTask(void *parameter);
bool connectWiFi();
void startAP();
void startWebServer();
void initLittleFS();
void saveCredentials(fs::FS &fs, const char *path, const char *message);
String loadCredentials(fs::FS &fs, const char *path);

/***********
* StepperTask - Free RTOS Task
* Handles stepper motor controls
* Recieves speed and on/off data via a queue/struct
* Controls stepper via GPIO directly, no stepper specific libraries
*************/
void StepperTask(void *parameter) {
  StepperJob job;
  for (;;) {
    if (xQueueReceive(stepperQueue, &job, portMAX_DELAY) == pdTRUE) {
      int spd = constrain(job.speed, MIN_SPEED, MAX_SPEED);
      bool treat = false;
      digitalWrite(dir, HIGH);
      for (int i = 0; i < STEPS_PER_REV; ++i) {
        digitalWrite(stepp, HIGH);
        delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
        digitalWrite(stepp, LOW);
        delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
        breakState = digitalRead(breakBeam);
        if (breakState == LOW){
          treat = true;
          Serial.println("Stepper run complete.");
          break;
        }
      }
      int count = 0;
      while (!treat && count < 5){
          digitalWrite(dir, HIGH);
          for (int i = 0; i < STEPS_PER_REV/DIVISOR; ++i){
            
            digitalWrite(stepp, HIGH);
            delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
            digitalWrite(stepp, LOW);
            delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
            breakState = digitalRead(breakBeam);
            if (breakState == LOW){
              treat = true;
              break;
              }
          }
          digitalWrite(dir, LOW);
          for (int i = 0; i < STEPS_PER_REV/DIVISOR; ++i){
            digitalWrite(stepp, HIGH);
            delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
            digitalWrite(stepp, LOW);
            delayMicroseconds(STEP_DELAY_BASE-(spd*1000));
            breakState = digitalRead(breakBeam);
            if (breakState == LOW){
              treat = true;
              break;
              }
          }
          count++;
      }
      Serial.println("Stepper run complete.");
    }
  }
}

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

/*******
* Sets up pins as outputs
* Initializes file system
* Creates tasks
*********/
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(stepp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(breakBeam, INPUT);
  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());
  initLittleFS();

  stepperQueue = xQueueCreate(1, sizeof(StepperJob));

  if (stepperQueue == NULL) {
    Serial.println("Failed to create stepperQueue!");
  }

  xTaskCreatePinnedToCore(
    StepperTask,
    "StepperTask",
    4096,
    NULL,
    2,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    WiFiTask,
    "WiFiTask",
    4096,
    NULL,
    1,
    &TaskWiFiHandle,
    0);
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
* Starts access point at: 192.168.4.1
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
  dnsServer.start(53, "*", WiFi.softAPIP()); 
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
  request->redirect("http://192.168.4.1/");
});
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request){
  request->redirect("/");
});

server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
  request->redirect("/");
});

server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request){
  request->redirect("/");
});
  /****
  * Runs stepper:
  * sets speed (1-19)
  *****/
  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
    int spd = MIN_SPEED;
    if (request->hasParam("speed")) {
      spd = constrain(request->getParam("speed")->value().toInt(), MIN_SPEED, MAX_SPEED);
    }
    StepperJob job;
    job.speed = spd;
    xQueueSend(stepperQueue, &job, 0);
    request->send(200, "text/plain", "Stepper command updated");
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
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
  request->redirect("/");
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

/*******
* Just moves the DNS along
********/
void loop() {
 if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}
