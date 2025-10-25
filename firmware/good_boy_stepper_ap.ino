#include <WiFi.h>
#include <Stepper.h>
#include <SPIFFS.h>

// ------------------- Access Point Setup -------------------
const char* ap_ssid = "GoodBoy";
const char* ap_password = "buddythedog";
WiFiServer server(80);

// ------------------- Stepper Setup -------------------
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 5, 19, 18, 21);

// ------------------- Helper Functions -------------------
void sendWebPage(WiFiClient &client);
void handleRunCommand(WiFiClient &client, const String &query);

void setup() {
  Serial.begin(115200);
  myStepper.setSpeed(15);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.println();
  Serial.println("Access Point started!");
  Serial.print("Connect to: ");
  Serial.println(ap_ssid);
  Serial.print("Open browser at: http://");
  Serial.println(IP);

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New client connected");
  String req = "";
  unsigned long startTime = millis();
  while (client.connected() && millis() - startTime < 2000) {
    if (client.available()) {
      char c = client.read();
      req += c;
      if (req.endsWith("\r\n\r\n")) break;
    }
  }

  if (req.startsWith("GET /run?")) {
    // Parse query
    int qStart = req.indexOf('?');
    int qEnd = req.indexOf(' ', qStart);
    String query = req.substring(qStart + 1, qEnd);
    handleRunCommand(client, query);
  } else {
    sendWebPage(client);
  }

  client.stop();
  Serial.println("Client disconnected\n");
}

// ------------------- Command Handler -------------------
void handleRunCommand(WiFiClient &client, const String &query) {
  String dir = "forward";
  int speedVal = 15;

  int idxDir = query.indexOf("dir=");
  if (idxDir != -1) {
    int end = query.indexOf('&', idxDir);
    if (end == -1) end = query.length();
    dir = query.substring(idxDir + 4, end);
  }

  int idxSpeed = query.indexOf("speed=");
  if (idxSpeed != -1) {
    int end = query.indexOf('&', idxSpeed);
    if (end == -1) end = query.length();
    speedVal = query.substring(idxSpeed + 6, end).toInt();
    if (speedVal <= 0 || speedVal > 15) speedVal = 15;
  }

  Serial.printf("Running one revolution (%s, %d RPM)\n", dir.c_str(), speedVal);
  myStepper.setSpeed(speedVal);

  // Add small pause before switching direction
  delay(100);

  // Force full revolution in selected direction
  if (dir == "reverse") {
    for (int i = 0; i < stepsPerRevolution; i++) {
      myStepper.step(-1);
      delayMicroseconds(1000);
    }
  } else {
    for (int i = 0; i < stepsPerRevolution; i++) {
      myStepper.step(1);
      delayMicroseconds(1000);
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("Stepper done (1 rev, " + dir + ", " + String(speedVal) + " RPM)");
}

// ------------------- Web Interface -------------------
void sendWebPage(WiFiClient &client) {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-type:text/plain");
    client.println("Connection: close");
    client.println();
    client.println("index.html not found");
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  while (file.available()) {
    client.write(file.read());
  }
  file.close();
}
