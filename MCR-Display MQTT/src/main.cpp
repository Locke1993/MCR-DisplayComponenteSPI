#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <time.h>  // NEU: Für NTP

// --- CONFIG ---
const char* WIFI_SSID = "Galaxy A22 5G4921";
const char* WIFI_PASS = "hi12345678.";

// Cloud-MQTT-Broker (TLS/SSL) - aus secrets.h
const char* MQTT_BROKER = "h2a8a107.ala.eu-central-1.emqxsl.com";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "Timo";
const char* MQTT_USER = "Timo";  // NICHT ÄNDERN!
const char* MQTT_PASS = "lockerflocker";  // NICHT ÄNDERN!

// MQTT Topics vom Sender (2 Sender: Lukas + Niklas)
const char* TOPIC_LUKAS = "smarthome/senderlukas/env";
const char* TOPIC_NIKLAS = "smarthome/senderniklas/env";

// NTP Server - NEU
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;      // GMT+1 (Deutschland Winter)
const int DAYLIGHT_OFFSET_SEC = 0;     // Sommerzeit: 3600, Winter: 0

// Display pins
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_BL   15

// Display layout constants
constexpr int LABEL_BASE_Y_OFFSET = 75;
constexpr int TEMP_ROW_Y_OFFSET = 0;
constexpr int HUM_ROW_Y_OFFSET = 32;
constexpr int PRESS_ROW_Y_OFFSET = 64;
constexpr int LEFT_VAL_X_OFFSET = 140;
constexpr int RIGHT_VAL_X_OFFSET = 225;
constexpr int HEADER_Y_OFFSET = -24;
constexpr int COL_PADDING = 24;

// Status LED positions (unten links und rechts)
constexpr int LED_SIZE = 16;
constexpr int LED_Y = 220;
constexpr int MQTT_LED_X = 20;
constexpr int WIFI_LED_X = 290;

// Timing constants
const unsigned long DISPLAY_REFRESH_MS = 1000;
const unsigned long WIFI_STATUS_PRINT_MS = 3000;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;
const unsigned long MQTT_DATA_TIMEOUT_MS = 120000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 500;
const int MAX_WIFI_ATTEMPTS = 30;

// Padding sizes
const int DATE_PAD = 26;
const int TIME_PAD = 8;

// Root-CA Zertifikat (DigiCert Global Root CA)
static const char ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)PEM";

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
RTC_DS3231 rtc;
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

// Stored sensor values (Spalte 1 = Lukas, Spalte 2 = Niklas)
float temp1 = NAN, hum1 = NAN, pres1 = NAN;
float temp2 = NAN, hum2 = NAN, pres2 = NAN;

// Timing variables
unsigned long lastDisplayUpdate = 0;
unsigned long lastWifiPrint = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastMqttData = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastNTPSync = 0;  // NEU: Letzte NTP-Synchronisation

bool rtc_ok = false;
bool wifiConnecting = false;
int wifiAttempts = 0;

const char* weekdayLongDE[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};

void padRight(char* dest, size_t destSize, const char* src, int pad) {
  snprintf(dest, destSize, "%-*s", pad, src);
}

String payloadToString(byte* payload, unsigned int length) {
  String s;
  s.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) s += (char)payload[i];
  s.trim();
  return s;
}

// DEBUG Einstellungen
#define DEBUG_ENABLED true  // Auf false setzen um Debug zu deaktivieren
#define DEBUG_MQTT true
#define DEBUG_WIFI true
#define DEBUG_DISPLAY true
#define DEBUG_RTC true

// Debug-Makros
#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#if DEBUG_MQTT && DEBUG_ENABLED
  #define DEBUG_MQTT_PRINT(x) Serial.print(x)
  #define DEBUG_MQTT_PRINTLN(x) Serial.println(x)
  #define DEBUG_MQTT_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_MQTT_PRINT(x)
  #define DEBUG_MQTT_PRINTLN(x)
  #define DEBUG_MQTT_PRINTF(...)
#endif

#if DEBUG_WIFI && DEBUG_ENABLED
  #define DEBUG_WIFI_PRINT(x) Serial.print(x)
  #define DEBUG_WIFI_PRINTLN(x) Serial.println(x)
  #define DEBUG_WIFI_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_WIFI_PRINT(x)
  #define DEBUG_WIFI_PRINTLN(x)
  #define DEBUG_WIFI_PRINTF(...)
#endif

#if DEBUG_DISPLAY && DEBUG_ENABLED
  #define DEBUG_DISP_PRINT(x) Serial.print(x)
  #define DEBUG_DISP_PRINTLN(x) Serial.println(x)
  #define DEBUG_DISP_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_DISP_PRINT(x)
  #define DEBUG_DISP_PRINTLN(x)
  #define DEBUG_DISP_PRINTF(...)
#endif

#if DEBUG_RTC && DEBUG_ENABLED
  #define DEBUG_RTC_PRINT(x) Serial.print(x)
  #define DEBUG_RTC_PRINTLN(x) Serial.println(x)
  #define DEBUG_RTC_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_RTC_PRINT(x)
  #define DEBUG_RTC_PRINTLN(x)
  #define DEBUG_RTC_PRINTF(...)
#endif

// NEU: NTP-Synchronisation mit RTC
void syncRTCWithNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_RTC_PRINTLN("[NTP] ✗ WiFi not connected - skipping sync");
    return;
  }
  
  if (!rtc_ok) {
    DEBUG_RTC_PRINTLN("[NTP] ✗ RTC not initialized - skipping sync");
    return;
  }
  
  DEBUG_RTC_PRINTLN("[NTP] Syncing time from internet...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {  // 10 Sekunden Timeout
    DEBUG_RTC_PRINTLN("[NTP] ✗ Failed to get time from NTP server");
    return;
  }
  
  // NTP-Zeit in RTC schreiben
  rtc.adjust(DateTime(timeinfo.tm_year + 1900, 
                      timeinfo.tm_mon + 1, 
                      timeinfo.tm_mday,
                      timeinfo.tm_hour, 
                      timeinfo.tm_min, 
                      timeinfo.tm_sec));
  
  DEBUG_RTC_PRINTF("[NTP] ✓ Time synced: %02d.%02d.%04d %02d:%02d:%02d\n",
                   timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  lastNTPSync = millis();
}

// Status-LEDs zeichnen (WiFi + MQTT) mit Beschriftung
void drawStatusLEDs() {
  DEBUG_DISP_PRINTLN("[DISP] Drawing status LEDs...");
  
  // MQTT LED + Label (links unten)
  uint16_t mqttColor;
  if (!mqtt.connected()) {
    mqttColor = ILI9341_RED;
    DEBUG_MQTT_PRINTLN("[LED] MQTT: RED (disconnected)");
  } else if (millis() - lastMqttData > MQTT_DATA_TIMEOUT_MS && lastMqttData > 0) {
    mqttColor = ILI9341_YELLOW;
    DEBUG_MQTT_PRINTLN("[LED] MQTT: YELLOW (no data)");
  } else {
    mqttColor = ILI9341_GREEN;
    DEBUG_MQTT_PRINTLN("[LED] MQTT: GREEN (connected + data)");
  }
  tft.fillRect(MQTT_LED_X, LED_Y, LED_SIZE, LED_SIZE, mqttColor);
  tft.drawRect(MQTT_LED_X, LED_Y, LED_SIZE, LED_SIZE, ILI9341_WHITE);
  
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(MQTT_LED_X + 3, LED_Y - 10);
  tft.print("MQTT");
  
  // WiFi LED + Label (rechts unten)
  uint16_t wifiColor = (WiFi.status() == WL_CONNECTED) ? ILI9341_GREEN : ILI9341_RED;
  DEBUG_WIFI_PRINTF("[LED] WiFi: %s\n", wifiColor == ILI9341_GREEN ? "GREEN" : "RED");
  
  tft.fillRect(WIFI_LED_X, LED_Y, LED_SIZE, LED_SIZE, wifiColor);
  tft.drawRect(WIFI_LED_X, LED_Y, LED_SIZE, LED_SIZE, ILI9341_WHITE);
  
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(WIFI_LED_X + 3, LED_Y - 10);
  tft.print("WiFi");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String tpc(topic);
  String msg = payloadToString(payload, length);
  
  DEBUG_MQTT_PRINTF("[MQTT] Received on [%s]: %s\n", topic, msg.c_str());

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    DEBUG_MQTT_PRINTF("[MQTT] ✗ JSON Parse Error: %s\n", error.c_str());
    return;
  }

  if (tpc == TOPIC_LUKAS) {
    // GEÄNDERT: "temp" statt "t280", "hum" statt "rh280", "pres" statt "p280"
    if (doc.containsKey("temp")) temp1 = doc["temp"];
    if (doc.containsKey("hum")) hum1 = doc["hum"];
    if (doc.containsKey("pres")) pres1 = doc["pres"];
    DEBUG_MQTT_PRINTF("[MQTT] Lukas: T=%.2f°C, H=%.2f%%, P=%.2fhPa\n", temp1, hum1, pres1);
  } 
  else if (tpc == TOPIC_NIKLAS) {
    // GEÄNDERT: "temp" statt "t280", "hum" statt "rh280", "pres" statt "p280"
    if (doc.containsKey("temp")) temp2 = doc["temp"];
    if (doc.containsKey("hum")) hum2 = doc["hum"];
    if (doc.containsKey("pres")) pres2 = doc["pres"];
    DEBUG_MQTT_PRINTF("[MQTT] Niklas: T=%.2f°C, H=%.2f%%, P=%.2fhPa\n", temp2, hum2, pres2);
  }

  lastMqttData = millis();
  DEBUG_MQTT_PRINTF("[MQTT] Last data timestamp: %lu\n", lastMqttData);
}

void tryMqttConnect() {
  if (mqtt.connected()) return;
  
  const unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttAttempt = now;
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_MQTT_PRINTLN("[MQTT] ✗ WiFi not connected - skipping MQTT connect");
    return;
  }

  DEBUG_MQTT_PRINTLN("[MQTT] Setting up TLS...");
  espClient.setCACert(ROOT_CA);
  espClient.setTimeout(15);
  
  mqtt.setCallback(mqttCallback);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(60);
  mqtt.setSocketTimeout(15);
  mqtt.setBufferSize(1024);
  
  DEBUG_MQTT_PRINTF("[MQTT] Connecting to: %s:%d\n", MQTT_BROKER, MQTT_PORT);
  DEBUG_MQTT_PRINTF("[MQTT] Client ID: %s | User: %s\n", MQTT_CLIENT_ID, MQTT_USER);
  
  if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, "status/timodisplay", 0, true, "offline")) {
    DEBUG_MQTT_PRINTLN("[MQTT] ✓ Connected!");
    
    mqtt.publish("status/timodisplay", "online", true);
    DEBUG_MQTT_PRINTLN("[MQTT] Published status: online");
    
    bool sub1 = mqtt.subscribe(TOPIC_LUKAS);
    bool sub2 = mqtt.subscribe(TOPIC_NIKLAS);
    
    DEBUG_MQTT_PRINTF("[MQTT] Subscribe %s: %s\n", TOPIC_LUKAS, sub1 ? "✓ OK" : "✗ FAILED");
    DEBUG_MQTT_PRINTF("[MQTT] Subscribe %s: %s\n", TOPIC_NIKLAS, sub2 ? "✓ OK" : "✗ FAILED");
  } else {
    int state = mqtt.state();
    DEBUG_MQTT_PRINTF("[MQTT] ✗ Connect failed, rc=%d → ", state);
    
    switch(state) {
      case -4: DEBUG_MQTT_PRINTLN("Connection timeout"); break;
      case -3: DEBUG_MQTT_PRINTLN("Connection lost"); break;
      case -2: DEBUG_MQTT_PRINTLN("Connect failed (Network/TLS)"); break;
      case -1: DEBUG_MQTT_PRINTLN("Disconnected"); break;
      case 1: DEBUG_MQTT_PRINTLN("Bad protocol version"); break;
      case 2: DEBUG_MQTT_PRINTLN("Server rejected client ID"); break;
      case 3: DEBUG_MQTT_PRINTLN("Server unavailable"); break;
      case 4: DEBUG_MQTT_PRINTLN("Bad credentials"); break;
      case 5: DEBUG_MQTT_PRINTLN("NOT AUTHORIZED (ACL missing!)"); break;
      default: DEBUG_MQTT_PRINTLN("Unknown error"); break;
    }
  }
}

void tryWifiConnect() {
  const unsigned long now = millis();
  
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiConnecting) {
      DEBUG_WIFI_PRINTF("[WiFi] ✓ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      wifiConnecting = false;
      wifiAttempts = 0;
      
      // NEU: Erste NTP-Sync direkt nach WiFi-Verbindung
      if (lastNTPSync == 0) {
        delay(1000);  // Kurz warten bis Verbindung stabil
        syncRTCWithNTP();
      }
    }
    return;
  }
  
  if (wifiConnecting) {
    if (now - lastWifiAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
      lastWifiAttempt = now;
      wifiAttempts++;
      DEBUG_WIFI_PRINT("");
      
      if (wifiAttempts >= MAX_WIFI_ATTEMPTS) {
        DEBUG_WIFI_PRINTLN("\n[WiFi] ✗ Timeout - retrying...");
        WiFi.disconnect();
        delay(100);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        wifiAttempts = 0;
      }
    }
    return;
  }
  
  if (now - lastWifiAttempt >= 10000) {
    lastWifiAttempt = now;
    wifiAttempts = 0;
    wifiConnecting = true;
    
    DEBUG_WIFI_PRINTF("[WiFi] Connecting to: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

void drawStaticLabels() {
  DEBUG_DISP_PRINTLN("[DISP] Drawing static labels...");
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  
  const int baseY = LABEL_BASE_Y_OFFSET;
  
  tft.setCursor(8, baseY); tft.print("Temp.:");
  tft.setCursor(8, baseY + 32); tft.print("Feuchte:");
  tft.setCursor(8, baseY + 64); tft.print("Druck:");
  
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(160, baseY + HEADER_Y_OFFSET); tft.print("LUKAS");
  
  tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
  tft.setCursor(240, baseY + HEADER_Y_OFFSET); tft.print("NIKLAS");
}

void drawDateTime() {
  if (!rtc_ok) {
    DEBUG_RTC_PRINTLN("[RTC] ✗ Not initialized - showing error");
    
    char datepad[DATE_PAD + 1];
    padRight(datepad, sizeof(datepad), "RTC N/A", DATE_PAD);
    
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(datepad, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((tft.width() - w) / 2, 6);
    tft.print(datepad);
    
    char timepad[TIME_PAD + 1];
    padRight(timepad, sizeof(timepad), "        ", TIME_PAD);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.getTextBounds(timepad, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((tft.width() - w) / 2, 22);
    tft.print(timepad);
    return;
  }

  DateTime now = rtc.now();
  const uint8_t dow = now.dayOfTheWeek();
  const char* wname = weekdayLongDE[dow];
  
  DEBUG_RTC_PRINTF("[RTC] %s %02d.%02d.%04d %02d:%02d:%02d\n", 
                   wname, now.day(), now.month(), now.year(),
                   now.hour(), now.minute(), now.second());
  
  char datebuf[32];
  snprintf(datebuf, sizeof(datebuf), "%s %02d.%02d.%04d", wname, now.day(), now.month(), now.year());
  char datepad[DATE_PAD + 1];
  padRight(datepad, sizeof(datepad), datebuf, DATE_PAD);
  
  char timebuf[16];
  snprintf(timebuf, sizeof(timebuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  char timepad[TIME_PAD + 1];
  padRight(timepad, sizeof(timepad), timebuf, TIME_PAD);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(datepad, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - w) / 2, 6);
  tft.print(datepad);
  
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.getTextBounds(timepad, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - w) / 2, 22);
  tft.print(timepad);
}

void drawCenteredValue(int x, int y, const char* format, float value, uint16_t color, int textSize) {
  char vs[32];
  tft.setTextSize(textSize);
  tft.setTextColor(color, ILI9341_BLACK);
  
  if (!isnan(value)) {
    snprintf(vs, sizeof(vs), format, value);
  } else {
    snprintf(vs, sizeof(vs), textSize == 3 ? "  ---  " : "  ---  ");
  }
  
  tft.setCursor(x, y);
  tft.print(vs);
}

void drawSensors() {
  DEBUG_DISP_PRINTLN("[DISP] Drawing sensor values...");
  DEBUG_DISP_PRINTF("[DISP] Lukas: T=%.1f, H=%.1f%%, P=%.1f\n", temp1, hum1, pres1);
  DEBUG_DISP_PRINTF("[DISP] Niklas: T=%.1f, H=%.1f%%, P=%.1f\n", temp2, hum2, pres2);
  
  const int baseY = LABEL_BASE_Y_OFFSET;

  drawCenteredValue(LEFT_VAL_X_OFFSET, baseY + TEMP_ROW_Y_OFFSET, "%5.1f", temp1, ILI9341_GREEN, 3);
  drawCenteredValue(LEFT_VAL_X_OFFSET, baseY + HUM_ROW_Y_OFFSET, "%5.1f%%", hum1, ILI9341_GREEN, 2);
  drawCenteredValue(LEFT_VAL_X_OFFSET, baseY + PRESS_ROW_Y_OFFSET, "%6.1f", pres1, ILI9341_GREEN, 2);

  drawCenteredValue(RIGHT_VAL_X_OFFSET, baseY + TEMP_ROW_Y_OFFSET, "%5.1f", temp2, ILI9341_MAGENTA, 3);
  drawCenteredValue(RIGHT_VAL_X_OFFSET, baseY + HUM_ROW_Y_OFFSET, "%5.1f%%", hum2, ILI9341_MAGENTA, 2);
  drawCenteredValue(RIGHT_VAL_X_OFFSET, baseY + PRESS_ROW_Y_OFFSET, "%6.1f", pres2, ILI9341_MAGENTA, 2);
}

void updateDisplay() {
  DEBUG_DISP_PRINTLN("[DISP] --- Updating display ---");
  drawDateTime();
  drawSensors();
  drawStatusLEDs();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  DEBUG_PRINTLN("\n\n========================================");
  DEBUG_PRINTLN("=== MCR Display MQTT Empfänger (TLS) ===");
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTF("Debug enabled: %s\n", DEBUG_ENABLED ? "YES" : "NO");
  DEBUG_PRINTF("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  DEBUG_PRINTLN("[INIT] Starting SPI...");
  SPI.begin(TFT_SCLK, -1, TFT_MOSI);
  
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    DEBUG_PRINTLN("[INIT] Display backlight ON");
  }
  
  DEBUG_PRINTLN("[INIT] Initializing TFT display...");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  drawStaticLabels();
  DEBUG_PRINTLN("[INIT] Display initialized ✓");

  DEBUG_PRINTLN("[INIT] Starting I2C for RTC...");
  Wire.begin(21, 22);
  rtc_ok = rtc.begin();
  
  if (rtc_ok) {
    DEBUG_PRINTLN("[INIT] RTC initialized ✓");
    if (rtc.lostPower()) {
      DEBUG_PRINTLN("[RTC] ⚠ Lost power - will sync with NTP when WiFi connects");
    }
    DateTime now = rtc.now();
    DEBUG_RTC_PRINTF("[RTC] Current time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  } else {
    DEBUG_PRINTLN("[INIT] ✗ RTC initialization failed!");
  }

  updateDisplay();
  lastDisplayUpdate = millis();
  lastMqttData = 0;
  lastNTPSync = 0;

  DEBUG_WIFI_PRINTF("[WiFi] Connecting to: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifiConnecting = true;
  lastWifiAttempt = millis();
  wifiAttempts = 0;
  
  lastWifiPrint = millis();
  lastMqttAttempt = 0;
  
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("Setup complete - entering main loop");
  DEBUG_PRINTLN("========================================\n");
}

void loop() {
  const unsigned long now = millis();
  
  tryWifiConnect();
  
  // NEU: NTP-Synchronisation alle 24 Stunden (oder beim ersten Mal)
  if (WiFi.status() == WL_CONNECTED && rtc_ok) {
    if (lastNTPSync == 0 || (now - lastNTPSync > 86400000)) {  // 86400000ms = 24 Stunden
      syncRTCWithNTP();
    }
  }
  
  if (now - lastWifiPrint >= WIFI_STATUS_PRINT_MS) {
    lastWifiPrint = now;
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTF("[STATUS] WiFi: ✓ (%s) | MQTT: %s | Heap: %d\n", 
                    WiFi.localIP().toString().c_str(),
                    mqtt.connected() ? "✓" : "✗",
                    ESP.getFreeHeap());
    } else {
      DEBUG_WIFI_PRINTF("[STATUS] WiFi: ✗ (status=%d, connecting=%s)\n", 
                    WiFi.status(), 
                    wifiConnecting ? "yes" : "no");
    }
  }

  tryMqttConnect();
  if (mqtt.connected()) {
    mqtt.loop();
  }

  if (now - lastDisplayUpdate >= DISPLAY_REFRESH_MS) {
    lastDisplayUpdate = now;
    updateDisplay();
  }

  yield();
}