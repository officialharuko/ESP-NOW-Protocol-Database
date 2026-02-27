#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <functional>
#include "time.h"

// ================== WIFI-KUN ==================
const char* ssid     = "ZTE_2.4G_yTt4CG";
const char* password = "C4CD3d2T";
const char* ntpServer = "pool.ntp.org";

const long  gmtOffset_sec = 28800; 
const int   daylightOffset_sec = 0;

uint8_t receiverAddress[] = {0x84, 0x1F, 0xE8, 0x69, 0xD3, 0x24};
WebServer server(80);

static const char* DATA_PATH = "/data.json";
static const char* CNT_PATH  = "/id_counter.txt";

// ================== Helpers ==================
String getTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "0000-00-00 00:00:00";
  char buffer[25];
  strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

uint32_t nextId() {
  uint32_t val = 0;
  if (SPIFFS.exists(CNT_PATH)) {
    File f = SPIFFS.open(CNT_PATH, FILE_READ);
    if (f) { val = f.readString().toInt(); f.close(); }
  }
  val++;
  File w = SPIFFS.open(CNT_PATH, FILE_WRITE);
  if (w) { w.print(val); w.close(); }
  return val;
}

String htmlMainLink() { return String("<p><a href='/'>Back to Main Menu</a></p>"); }

String roomSelectHTML(int selected) {
  String html = "<select name='room'>";
  for (int i = 1; i <= 10; i++) {
    html += "<option value='" + String(i) + "'";
    if (i == selected) html += " selected";
    html += ">Room " + String(i) + "</option>";
  }
  html += "</select>";
  return html;
}

String actionSelectHTML(const String& selected) {
  struct Opt { const char* val; const char* label; };
  Opt options[] = {
    {"open_door", "Open Door"},
    {"adjust_aircon", "Adjust Aircon"},
    {"emergency", "Emergency"},
    {"notify_guard", "Notify Guard: Go to Room"}
  };
  String html = "<select name='action'>";
  for (auto &op : options) {
    html += "<option value='" + String(op.val) + "'";
    if (selected == op.val) html += " selected";
    html += ">" + String(op.label) + "</option>";
  }
  html += "</select>";
  return html;
}

bool rewriteFileTransform(const char* path, std::function<bool(JsonDocument&)> transformKeep) {
  File in = SPIFFS.open(path, FILE_READ);
  File out = SPIFFS.open("/tmp.json", FILE_WRITE);
  if (!out) { if (in) in.close(); return false; }
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

// ================== ESP-NOW ==================
void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS: Sent OK" : "ERROR: Send Failed");
}

void sendAllViaEspNow() {
  File file = SPIFFS.open(DATA_PATH, FILE_READ);
  if (!file) return;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      esp_now_send(receiverAddress, (uint8_t*)line.c_str(), line.length());
      delay(150);
    }
  }
  file.close();
}

// ================== Routes ==================
void handleHome() {
  String html = "<h2>ESP-A Main Menu</h2><ul>";
  html += "<li><a href='/add'>Add Person</a></li>";
  html += "<li><a href='/view'>View Database</a></li>";
  html += "<li><a href='/send'>Send to ESP-B</a></li>";
  html += "<li><a href='/clear'>Clear Storage</a></li></ul>";
  server.send(200, "text/html", html);
}

void handleAddForm() {
  String html = "<h2>Add New Entry</h2><form method='POST' action='/add'>";
  html += "Name: <input name='name'><br>Occupation: <input name='occupation'><br>";
  html += "Grade: <input name='grade'><br>Room: " + roomSelectHTML(1) + "<br>";
  html += "Action: " + actionSelectHTML("open_door") + "<br>";
  html += "<input type='submit' value='Save'></form>" + htmlMainLink();
  server.send(200, "text/html", html);
}

void handleAdd() {
  StaticJsonDocument<512> doc;
  doc["id"] = nextId();
  doc["name"] = server.arg("name");
  doc["occupation"] = server.arg("occupation");
  doc["grade"] = server.arg("grade");
  doc["room"] = server.arg("room").toInt();
  doc["action"] = server.arg("action");
  doc["timestamp"] = getTimestamp();

  File f = SPIFFS.open(DATA_PATH, FILE_APPEND);
  if (f) { serializeJson(doc, f); f.println(); f.close(); }
  server.sendHeader("Location", "/view");
  server.send(303);
}

void handleView() {
  String q = server.arg("q");
  String ql = q; ql.toLowerCase();
  String html = "<h2>ESP-A Database</h2><form action='/view'>Search: <input name='q' placeholder='Name or Room' value='"+q+"'><input type='submit' value='Go'></form>";
  html += "<table border='1' cellpadding='5'><tr><th>ID</th><th>Time</th><th>Name</th><th>Room</th><th>Operation</th></tr>";

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
        html += "<tr><td>"+String(id)+"</td><td>"+String(doc["timestamp"]|"")+"</td><td>"+n+"</td><td>Room "+rNum+"</td>";
        html += "<td><a href='/delete?id="+String(id)+"'>Delete</a></td></tr>";
      }
    }
    file.close();
  }
  html += "</table>" + htmlMainLink();
  server.send(200, "text/html", html);
}

void handleDelete() {
  uint32_t target = server.arg("id").toInt();
  rewriteFileTransform(DATA_PATH, [target](JsonDocument& doc) { return (uint32_t)doc["id"] != target; });
  server.sendHeader("Location", "/view");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  SPIFFS.begin(true);

  Serial.println("\n----------------------------------");
  Serial.println("ESP-A Web Server");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  Serial.println("----------------------------------");

  server.on("/", handleHome);
  server.on("/add", HTTP_GET, handleAddForm);
  server.on("/add", HTTP_POST, handleAdd);
  server.on("/view", handleView);
  server.on("/delete", handleDelete);
  server.on("/send", [](){ sendAllViaEspNow(); server.send(200, "text/html", "Transfer Requested." + htmlMainLink()); });
  server.on("/clear", [](){ SPIFFS.remove(DATA_PATH); SPIFFS.remove(CNT_PATH); server.send(200, "text/html", "Cleared." + htmlMainLink()); });
  server.begin();

  if (esp_now_init() == ESP_OK) {
    esp_now_register_send_cb(onSent);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    esp_now_add_peer(&peerInfo);
  }
}

void loop() { server.handleClient(); }