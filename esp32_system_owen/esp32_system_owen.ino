/*
  Sources:

  freertos.org

  FreeRTOS API and Examples - Espressif Systems docs.espressif.com

  ChatGPT - OpenAI.com

  Rui Santos - RandomNerdTutorials.com

  Last Minute Engineers - https://lastminuteengineers.com/a4988-stepper-motor-driver-arduino-tutorial/

  ESP32 I/O - https://esp32io.com/tutorials/esp32-relay

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

#define RELAY_PIN 27
#define STEPS_PER_REV 200

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

String ssid, pass;
bool relayOn = false;

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
      int spd = job.speed;
	    digitalWrite(dir, HIGH);
      for (int i = 0; i < STEPS_PER_REV; ++i) {
        digitalWrite(stepp, HIGH);
		    delayMicroseconds(21000-(spd*1000));
        digitalWrite(stepp, LOW);
        delayMicroseconds(21000-(spd*1000));
        vTaskDelay(pdMS_TO_TICKS(1));
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
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(stepp, OUTPUT);
  pinMode(dir, OUTPUT);
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
  ssid = loadCredentials(LittleFS, ssidPath);
  pass = loadCredentials(LittleFS, passPath);
  
  if (connectWiFi()) {
    if (MDNS.begin("goodboy")) {
     Serial.println("mDNS started: http://goodboy.local");
    } 
    else {
      Serial.println("mDNS failed");
    }
    } 
  else {
    startAP();
  }

  startWebServer();
  
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
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(53, "*", WiFi.softAPIP()); 
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

/*******
* startWebServer - void funct
* Starts webpage, controls:
* relay (vacuum)
* stepper (dog treat dispenser)
* saving wifi credentials
********/
void startWebServer() {
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/");
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/relay/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    relayOn = !relayOn;
    digitalWrite(RELAY_PIN, relayOn ? HIGH : LOW);
    Serial.printf("Relay toggled -> %s\n", relayOn ? "ON" : "OFF");
    request->send(200, "text/plain", relayOn ? "Relay ON" : "Relay OFF");
  });

  /****
  * Runs stepper:
  * sets speed (1-20)
  *****/
  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request) {
    int spd = 1;
    if (request->hasParam("speed")) {
      spd = request->getParam("speed")->value().toInt();
    }
    StepperJob job;
    job.speed = abs(spd);
    xQueueOverwrite(stepperQueue, &job);
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
      [](void *arg)->void {
        RebootTask(arg);
      },
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

/*******
* Just moves the DNS along
********/
void loop() {
 if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}
