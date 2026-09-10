/*
  Homelab Display V6

  Hardware:
    Raspberry Pi Pico W / Pico 2 W
    Waveshare Pico-ResTouch-LCD-3.5
    Adafruit SHT41 temperature/humidity sensor

  SHT41:
    SDA -> GPIO 0
    SCL -> GPIO 1

  Public portfolio version:
    Replace all placeholder network/API values before compiling.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <TFT_eSPI.h>

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* PVE_HOST = "YOUR_PROXMOX_IP";
const int PVE_PORT = 8006;
const char* PVE_NODE = "pve";
const char* PVE_TOKEN_ID = "dashboard-reader@pve!display";
const char* PVE_TOKEN_SECRET = "YOUR_PROXMOX_API_TOKEN_SECRET";

const char* TEMP_API_HOST = "YOUR_PROXMOX_IP";
const int TEMP_API_PORT = 8765;

const int VMID_PIHOLE = 100;
const int VMID_WARROOM = 101;
const int VMID_SYNCTHING = 102;
const int VMID_JELLYFIN = 103;
const int VMID_RIPPER = 104;

const unsigned long REFRESH_INTERVAL = 10000;

#define SHT_SDA_PIN 0
#define SHT_SCL_PIN 1

Adafruit_SHT4x sht4;
TFT_eSPI tft = TFT_eSPI();

#define COLOR_BG TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_GREEN TFT_GREEN
#define COLOR_RED TFT_RED
#define COLOR_YELLOW TFT_YELLOW
#define COLOR_CYAN TFT_CYAN
#define COLOR_GRAY TFT_DARKGREY
#define COLOR_HEADER TFT_GREEN
#define COLOR_SEPARATOR 0x0320

struct DashboardData {
  bool pveOnline;
  int cpuPercent;
  int ramPercent;
  unsigned long uptimeSeconds;
  String pihole;
  String warroom;
  String syncthing;
  String jellyfin;
  String ripper;
  bool shtOnline;
  float roomTempC;
  float roomTempF;
  float humidity;
  bool serverTempOnline;
  float serverTempC;
};

DashboardData dashboard;
unsigned long lastRefresh = 0;

bool connectWiFi();
bool initSHT41();
bool readSHT41();
bool readServerTemp();
bool pveGet(const String& path, JsonDocument& doc);
bool updateNodeStatus();
bool updateVMStatus();
void updateDashboard();
String findVMStatus(JsonArray resources, int vmid);
String formatUptime(unsigned long seconds);
uint16_t statusColor(const String& status);
void drawDashboard();
void drawStatusRow(const char* label, const String& status, int y);
void drawConnecting();
void drawWiFiFailure();

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("================================");
  Serial.println(" HOMELAB DISPLAY V6");
  Serial.println("================================");

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
  tft.setTextWrap(false);
  drawConnecting();

  Wire.setSDA(SHT_SDA_PIN);
  Wire.setSCL(SHT_SCL_PIN);
  Wire.begin();

  if (initSHT41()) {
    Serial.println("SHT41 initialized.");
  } else {
    Serial.println("WARNING: SHT41 not detected.");
  }

  if (!connectWiFi()) {
    drawWiFiFailure();
    Serial.println("Wi-Fi connection failed.");
    return;
  }

  Serial.println("Wi-Fi connected.");
  Serial.print("Pico IP: ");
  Serial.println(WiFi.localIP());

  updateDashboard();
  drawDashboard();
  lastRefresh = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected.");
    drawConnecting();
    connectWiFi();
    delay(1000);
  }

  if (millis() - lastRefresh >= REFRESH_INTERVAL) {
    lastRefresh = millis();
    updateDashboard();
    drawDashboard();
  }

  delay(50);
}

bool initSHT41() {
  if (!sht4.begin(&Wire)) {
    dashboard.shtOnline = false;
    return false;
  }

  dashboard.shtOnline = true;
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  return true;
}

bool readSHT41() {
  sensors_event_t humidityEvent;
  sensors_event_t tempEvent;

  if (!dashboard.shtOnline && !initSHT41()) {
    return false;
  }

  bool success = sht4.getEvent(&humidityEvent, &tempEvent);

  if (!success) {
    dashboard.shtOnline = false;
    Serial.println("SHT41 read failed.");
    return false;
  }

  dashboard.shtOnline = true;
  dashboard.roomTempC = tempEvent.temperature;
  dashboard.roomTempF = (dashboard.roomTempC * 9.0 / 5.0) + 32.0;
  dashboard.humidity = humidityEvent.relative_humidity;

  Serial.print("Room: ");
  Serial.print(dashboard.roomTempF, 1);
  Serial.print(" F, RH: ");
  Serial.print(dashboard.humidity, 1);
  Serial.println(" %");

  return true;
}

bool readServerTemp() {
  if (WiFi.status() != WL_CONNECTED) {
    dashboard.serverTempOnline = false;
    return false;
  }

  WiFiClient client;
  HTTPClient http;

  String url = String("http://") + TEMP_API_HOST + ":" + String(TEMP_API_PORT) + "/temp";

  if (!http.begin(client, url)) {
    dashboard.serverTempOnline = false;
    return false;
  }

  int httpCode = http.GET();

  if (httpCode != 200) {
    dashboard.serverTempOnline = false;
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error || doc["cpu_temp"].isNull()) {
    dashboard.serverTempOnline = false;
    return false;
  }

  dashboard.serverTempC = doc["cpu_temp"].as<float>();
  dashboard.serverTempOnline = true;
  return true;
}

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

bool pveGet(const String& path, JsonDocument& doc) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient https;
  String url = String("https://") + PVE_HOST + ":" + String(PVE_PORT) + "/api2/json" + path;

  if (!https.begin(secureClient, url)) {
    return false;
  }

  String authHeader = String("PVEAPIToken=") + PVE_TOKEN_ID + "=" + PVE_TOKEN_SECRET;
  https.addHeader("Authorization", authHeader);

  int httpCode = https.GET();

  if (httpCode != 200) {
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  return !deserializeJson(doc, payload);
}

bool updateNodeStatus() {
  JsonDocument doc;
  String path = String("/nodes/") + PVE_NODE + "/status";

  if (!pveGet(path, doc)) {
    dashboard.pveOnline = false;
    return false;
  }

  JsonObject data = doc["data"];

  if (data.isNull()) {
    dashboard.pveOnline = false;
    return false;
  }

  dashboard.pveOnline = true;

  float cpu = data["cpu"] | 0.0;
  dashboard.cpuPercent = round(cpu * 100.0);

  unsigned long long ramUsed = data["memory"]["used"] | 0ULL;
  unsigned long long ramTotal = data["memory"]["total"] | 0ULL;

  dashboard.ramPercent = ramTotal > 0
    ? (int)(((double)ramUsed / (double)ramTotal) * 100.0)
    : -1;

  dashboard.uptimeSeconds = data["uptime"] | 0UL;
  return true;
}

bool updateVMStatus() {
  JsonDocument doc;

  if (!pveGet("/cluster/resources?type=vm", doc)) {
    dashboard.pihole = "UNKNOWN";
    dashboard.warroom = "UNKNOWN";
    dashboard.syncthing = "UNKNOWN";
    dashboard.jellyfin = "UNKNOWN";
    dashboard.ripper = "UNKNOWN";
    return false;
  }

  JsonArray resources = doc["data"].as<JsonArray>();

  if (resources.isNull()) {
    return false;
  }

  dashboard.pihole = findVMStatus(resources, VMID_PIHOLE);
  dashboard.warroom = findVMStatus(resources, VMID_WARROOM);
  dashboard.syncthing = findVMStatus(resources, VMID_SYNCTHING);
  dashboard.jellyfin = findVMStatus(resources, VMID_JELLYFIN);
  dashboard.ripper = findVMStatus(resources, VMID_RIPPER);
  return true;
}

String findVMStatus(JsonArray resources, int vmid) {
  for (JsonObject resource : resources) {
    if ((resource["vmid"] | -1) == vmid) {
      String status = String(resource["status"] | "unknown");
      status.toLowerCase();

      if (status == "running") return "ONLINE";
      if (status == "stopped") return "STOPPED";

      status.toUpperCase();
      return status;
    }
  }

  return "UNKNOWN";
}

void updateDashboard() {
  readSHT41();
  readServerTemp();

  if (WiFi.status() != WL_CONNECTED) {
    dashboard.pveOnline = false;
    return;
  }

  if (updateNodeStatus()) {
    updateVMStatus();
  } else {
    dashboard.pihole = "UNKNOWN";
    dashboard.warroom = "UNKNOWN";
    dashboard.syncthing = "UNKNOWN";
    dashboard.jellyfin = "UNKNOWN";
    dashboard.ripper = "UNKNOWN";
  }
}

uint16_t statusColor(const String& status) {
  if (status == "ONLINE") return COLOR_GREEN;
  if (status == "STOPPED") return COLOR_GRAY;
  if (status == "UNKNOWN") return COLOR_YELLOW;
  return COLOR_RED;
}

String formatUptime(unsigned long seconds) {
  unsigned long days = seconds / 86400UL;
  unsigned long hours = (seconds % 86400UL) / 3600UL;
  unsigned long minutes = (seconds % 3600UL) / 60UL;

  if (days > 0) {
    return String(days) + "d " + String(hours) + "h";
  }

  return String(hours) + "h " + String(minutes) + "m";
}

void drawConnecting() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_GREEN, COLOR_BG);
  tft.setTextSize(4);
  tft.setCursor(135, 90);
  tft.print("HOMELAB");
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(105, 165);
  tft.print("Connecting to Wi-Fi...");
}

void drawWiFiFailure() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_RED, COLOR_BG);
  tft.setTextSize(3);
  tft.setCursor(100, 100);
  tft.print("NETWORK ERROR");
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(80, 165);
  tft.print("Wi-Fi connection failed");
}

void drawStatusRow(const char* label, const String& status, int y) {
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(25, y);
  tft.print(label);

  uint16_t color = statusColor(status);
  tft.fillCircle(225, y + 7, 6, color);
  tft.setTextColor(color, COLOR_BG);
  tft.setCursor(250, y);
  tft.print(status);
}

void drawDashboard() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(COLOR_HEADER, COLOR_BG);
  tft.setTextSize(3);
  tft.setCursor(20, 10);
  tft.print("HOMELAB");

  tft.setTextColor(COLOR_GRAY, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(435, 16);
  tft.print("V6");

  tft.drawFastHLine(20, 42, 440, COLOR_SEPARATOR);

  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(25, 51);
  tft.print("SERVICES");

  drawStatusRow("PVE", dashboard.pveOnline ? "ONLINE" : "OFFLINE", 75);
  drawStatusRow("PIHOLE", dashboard.pihole, 98);
  drawStatusRow("JELLYFIN", dashboard.jellyfin, 121);
  drawStatusRow("SYNC", dashboard.syncthing, 144);
  drawStatusRow("WARROOM", dashboard.warroom, 167);
  drawStatusRow("RIPPER", dashboard.ripper, 190);

  tft.drawFastHLine(20, 215, 440, COLOR_SEPARATOR);

  tft.setTextColor(COLOR_GREEN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(25, 228);

  if (dashboard.pveOnline) {
    tft.print("CPU ");
    tft.print(dashboard.cpuPercent);
    tft.print("%");
  } else {
    tft.print("CPU --");
  }

  tft.setCursor(155, 228);

  if (dashboard.pveOnline && dashboard.ramPercent >= 0) {
    tft.print("RAM ");
    tft.print(dashboard.ramPercent);
    tft.print("%");
  } else {
    tft.print("RAM --");
  }

  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.setCursor(285, 228);

  if (dashboard.pveOnline) {
    tft.print("UP ");
    tft.print(formatUptime(dashboard.uptimeSeconds));
  } else {
    tft.print("UP --");
  }

  tft.drawFastHLine(20, 252, 440, COLOR_SEPARATOR);
  tft.setTextSize(2);
  tft.setCursor(25, 265);

  if (dashboard.serverTempOnline) {
    tft.setTextColor(COLOR_GREEN, COLOR_BG);
    tft.print("SERVER ");
    tft.print(dashboard.serverTempC, 1);
    tft.print("C");
  } else {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.print("SERVER --.-C");
  }

  tft.setCursor(210, 265);

  if (dashboard.shtOnline) {
    tft.setTextColor(COLOR_YELLOW, COLOR_BG);
    tft.print("ROOM ");
    tft.print(dashboard.roomTempF, 1);
    tft.print("F");
  } else {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.print("ROOM --.-F");
  }

  tft.setCursor(25, 293);

  if (dashboard.shtOnline) {
    tft.setTextColor(COLOR_CYAN, COLOR_BG);
    tft.print("HUMIDITY ");
    tft.print(dashboard.humidity, 1);
    tft.print("%");
  } else {
    tft.setTextColor(COLOR_RED, COLOR_BG);
    tft.print("HUMIDITY --.-%");
  }
}
