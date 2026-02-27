#include <WiFi.h>
#include <esp_now.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <functional>

const char* ssid     = "ZTE_2.4G_yTt4CG";
const char* password = "C4CD3d2T";

WebServer server(80);
static const char* DATA_PATH = "/people.json";

String htmlMainLink() { return String("<p><a href='/'>Back to Main Menu</a></p>"); }

bool rewriteFileTransform(const char* path, std::function<bool(JsonDocument&)> transformKeep) {
  File in = SPIFFS.open(path, FILE_READ);
  File out = SPIFFS.open("/tmp.json", FILE_WRITE);
  if (!out) return false;
  if (in) {
    while (in.available()) {
      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, in.readStringUntil('\n')) == DeserializationError::Ok) {
        if (transformKeep(doc)) { serializeJson(doc, out); out.println(); }
      }
    }
    in.close();
  }
  out.close();
  SPIFFS.remove(path);
  return SPIFFS.rename("/tmp.json", path);
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
    File f = SPIFFS.open(DATA_PATH, FILE_APPEND);
    if (f) { serializeJson(doc, f); f.println(); f.close(); }
    Serial.printf("SUCCESS: Received Record: %s\n", (const char*)(doc["name"]|""));
  }
}

void handleHome() {
  String q = server.arg("q");
  String ql = q; ql.toLowerCase();
  String html = "<h2>ESP-B Database Viewer</h2>";
  html += "<form action='/'>Search: <input name='q' placeholder='Name or Room' value='"+q+"'><input type='submit' value='Go'></form>";
  html += "<p><a href='/clear'>Clear Database</a></p>";
  html += "<table border='1' cellpadding='5'><tr><th>Time</th><th>Name</th><th>Room</th><th>Action</th><th>Ops</th></tr>";

  File file = SPIFFS.open(DATA_PATH, FILE_READ);
  if (file) {
    while (file.available()) {
      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, file.readStringUntil('\n')) == DeserializationError::Ok) {
        String n = doc["name"] | "Unknown";
        String rNum = String((int)doc["room"]);
        String nl = n; nl.toLowerCase();
        
        if (q != "" && nl.indexOf(ql) == -1 && rNum != q) continue;

        uint32_t id = doc["id"];
        html += "<tr><td>"+String(doc["timestamp"]|"")+"</td><td>"+n+"</td><td>Room "+rNum+"</td>";
        html += "<td>"+String(doc["action"]|"")+"</td>";
        html += "<td><a href='/delete?id="+String(id)+"'>Delete</a></td></tr>";
      }
    }
    file.close();
  }
  html += "</table>" + htmlMainLink();
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  SPIFFS.begin(true);

  Serial.println("\n----------------------------------");
  Serial.println("ESP-B Web Viewer");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  Serial.print("MAC Address: "); Serial.println(WiFi.macAddress());
  Serial.println("----------------------------------");

  server.on("/", handleHome);
  server.on("/delete", [](){ 
    uint32_t target = server.arg("id").toInt();
    rewriteFileTransform(DATA_PATH, [target](JsonDocument& doc) { return (uint32_t)doc["id"] != target; });
    server.sendHeader("Location", "/"); server.send(303);
  });
  server.on("/clear", [](){ SPIFFS.remove(DATA_PATH); server.send(200, "text/html", "Cleared." + htmlMainLink()); });
  server.begin();

  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(onDataRecv);
}

void loop() { server.handleClient(); }