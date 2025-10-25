#include <WiFi.h>
#include <Stepper.h>

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
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println(R"rawliteral(
<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
  <meta charset=\"utf-8\">
  <title>Good Boy</title>
  <style>
    body { font-family:Helvetica,Arial; text-align:center; background:#111827; margin:0; }
    header { background:#111827; color:white; padding:18px; font-size:22px; }
    .container { margin:40px auto; max-width:600px; padding:20px; background:#111827; border-radius:12px; box-shadow:0 4px 12px rgba(0,0,0,0.1);}    
    .bigbtn { background:#1d4ed8; color:white; border:none; padding:28px 48px; font-size:36px; border-radius:12px; cursor:pointer; width:100%; }
    .bigbtn:active { background:#1e40af; }
    .controls { margin-top:20px; display:flex; justify-content:space-around; flex-wrap:wrap; }
    label { display:block; margin-bottom:5px; font-size:14px; color:#666; }
    select, input { font-size:18px; padding:6px 10px; border-radius:8px; border:1px solid #ccc; }
    footer { margin-top:30px; font-size:14px; color:#333; }
  </style>
</head>
<body>
  <header>Good Boy</header>
  <div class=\"container\">
    <button class=\"bigbtn\" onclick=\"run()\">Dispense</button>
    <div class=\"controls\">
      <div>
        <label for=\"dir\">Direction</label>
        <select id=\"dir\">
          <option value=\"forward\">Forward</option>
          <option value=\"reverse\">Reverse</option>
        </select>
      </div>
      <div>
        <label for=\"speed\">Speed (RPM)</label>
        <input id=\"speed\" type=\"number\" min=\"5\" max=\"15\" value=\"15\">
      </div>
    </div>
    <footer id=\"status\">Status: Ready</footer>
  </div>
  <script>
    function run(){
      const dir=document.getElementById('dir').value;
      const speed=document.getElementById('speed').value;
      document.getElementById('status').innerText="Running...";
      fetch(`/run?dir=${dir}&speed=${speed}`)
        .then(r=>r.text())
        .then(t=>{document.getElementById('status').innerText=t;})
        .catch(()=>{document.getElementById('status').innerText="Error";});
    }
  </script>
</body>
</html>
)rawliteral");

  client.println();
}
