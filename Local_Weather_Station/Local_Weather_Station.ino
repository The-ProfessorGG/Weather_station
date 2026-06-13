#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DHTPIN D4
#define DHTTYPE DHT11
#define WIFI_TIMEOUT_MS 10000  // 10 second startup timeout

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
BH1750 lightMeter;

// ---------------- WIFI + API ----------------
const char* ssid     = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
const char* apiKey   = OPENWEATHER_API_KEY;
const char* city        = "Ballito";
const char* countryCode = "ZA";

// ---------------- TIMING ----------------
unsigned long lastSwitchTime    = 0;
unsigned long lastWifiRetryTime = 0;
const unsigned long switchInterval    = 30000;
const unsigned long wifiRetryInterval = 30000;  // retry every 30s if disconnected

bool showAPI      = false;
bool wifiEverConnected = false;

// ---------------- API DATA ----------------
String apiTemp      = "--";
String apiHumidity  = "--";
String apiPressure  = "--";
String apiCondition = "No data";

// ---------------- HELPERS ----------------
String extractJsonValue(String json, String key) {
  int keyIndex = json.indexOf(key);
  if (keyIndex == -1) return "--";
  int colonIndex = json.indexOf(':', keyIndex);
  if (colonIndex == -1) return "--";
  int startIndex = colonIndex + 1;
  while (startIndex < json.length() &&
         (json[startIndex] == ' ' || json[startIndex] == '\"'))
    startIndex++;
  int endIndex = startIndex;
  while (endIndex < json.length() &&
         json[endIndex] != ',' &&
         json[endIndex] != '}' &&
         json[endIndex] != '\"')
    endIndex++;
  return json.substring(startIndex, endIndex);
}

bool fetchAPIWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    apiTemp = "--"; apiHumidity = "--"; apiPressure = "--";
    apiCondition = "WiFi fail";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://api.openweathermap.org/data/2.5/weather?q=" +
               String(city) + "," + String(countryCode) +
               "&appid=" + String(apiKey) + "&units=metric";

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    apiTemp      = extractJsonValue(payload, "\"temp\"");
    apiHumidity  = extractJsonValue(payload, "\"humidity\"");
    apiPressure  = extractJsonValue(payload, "\"pressure\"");
    apiCondition = extractJsonValue(payload, "\"description\"");

    if (apiTemp == "--" || apiHumidity == "--" || apiPressure == "--") {
      apiTemp = "--"; apiHumidity = "--"; apiPressure = "--";
      apiCondition = "Parse fail";
      http.end();
      return false;
    }
    http.end();
    return true;
  } else {
    apiTemp = "--"; apiHumidity = "--"; apiPressure = "--";
    apiCondition = (httpCode > 0) ? "API fail" : "Conn fail";
    http.end();
    return false;
  }
}

// ---------------- STARTUP SCREEN WITH COUNTDOWN ----------------
void showStartupScreen(int secondsRemaining) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi");

  display.setCursor(0, 14);
  display.print(ssid);

  // Progress bar
  int totalWidth = 118;
  float progress = 1.0 - ((float)secondsRemaining / (WIFI_TIMEOUT_MS / 1000));
  int barWidth   = (int)(totalWidth * progress);
  display.drawRect(5, 30, totalWidth, 8, WHITE);
  if (barWidth > 0)
    display.fillRect(5, 30, barWidth, 8, WHITE);

  display.setCursor(0, 44);
  display.print("Timeout in: ");
  display.print(secondsRemaining);
  display.print("s");

  display.display();
}

void showNoWifiScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi not found.");
  display.setCursor(0, 12);
  display.println("Running on local");
  display.setCursor(0, 22);
  display.println("sensors only.");
  display.setCursor(0, 40);
  display.println("Retrying in bg...");
  display.display();
  delay(2000);
}

// ---------------- DISPLAY SCREENS ----------------
void drawCountdown() {
  unsigned long elapsed   = millis() - lastSwitchTime;
  unsigned long remaining = 0;
  if (elapsed < switchInterval)
    remaining = (switchInterval - elapsed) / 1000;
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(110, 0);
  display.print(remaining);
  display.print("s");
}

void showLocalScreen(float temperature, float humidity, float pressure, float lightLevel) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  // Show a small WiFi indicator if connected
  if (WiFi.status() == WL_CONNECTED)
    display.print("Local Wx [W]");
  else
    display.print("Local Wx");

  drawCountdown();

  display.setCursor(0, 16);
  display.print("Temp: "); display.print(temperature); display.println(" C");
  display.setCursor(0, 26);
  display.print("Hum:  "); display.print(humidity);    display.println(" %");
  display.setCursor(0, 36);
  display.print("Pres: "); display.print(pressure);    display.println(" hPa");
  display.setCursor(0, 46);
  display.print("Light:"); display.print(lightLevel, 0); display.println(" lx");

  display.display();
}

void showAPIScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("API Weather");

  drawCountdown();

  display.setCursor(0, 16);
  display.print("Temp: "); display.print(apiTemp);     display.println(" C");
  display.setCursor(0, 26);
  display.print("Hum:  "); display.print(apiHumidity); display.println(" %");
  display.setCursor(0, 36);
  display.print("Pres: "); display.print(apiPressure); display.println(" hPa");
  display.setCursor(0, 46);
  display.print("Cond:"); display.println(apiCondition);

  display.display();
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    for (;;) {}
  }

  if (!bmp.begin()) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("BMP180 failed");
    display.display();
    while (1) {}
  }

  lightMeter.begin();

  // --- Timed WiFi connect with countdown on OLED ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long wifiStart    = millis();
  int           timeoutSecs  = WIFI_TIMEOUT_MS / 1000;

  while (WiFi.status() != WL_CONNECTED) {
    unsigned long elapsed  = millis() - wifiStart;
    if (elapsed >= WIFI_TIMEOUT_MS) break;             // give up

    int secsLeft = timeoutSecs - (int)(elapsed / 1000);
    showStartupScreen(secsLeft);
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiEverConnected = true;
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    fetchAPIWeather();
  } else {
    Serial.println("\nWiFi timed out — running local only");
    WiFi.disconnect(false);   // keeps credentials so reconnect works
    showNoWifiScreen();
  }

  lastSwitchTime    = millis();
  lastWifiRetryTime = millis();
}

// ---------------- LOOP ----------------
void loop() {
  float humidity    = dht.readHumidity();
  float temperature = dht.readTemperature() - 5;
  float pressure    = bmp.readPressure() / 100.0;
  float lightLevel  = lightMeter.readLightLevel();

  unsigned long currentMillis = millis();

  // --- Background WiFi reconnection attempt ---
  if (WiFi.status() != WL_CONNECTED &&
      currentMillis - lastWifiRetryTime >= wifiRetryInterval) {
    lastWifiRetryTime = currentMillis;
    Serial.println("Retrying WiFi...");
    WiFi.begin(ssid, password);

    // Give it up to 5s to connect without blocking the display
    unsigned long retryStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - retryStart < 5000) {
      delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected!");
      wifiEverConnected = true;
      fetchAPIWeather();
    }
  }

  // --- Screen switch every 30s ---
  if (currentMillis - lastSwitchTime >= switchInterval) {
    lastSwitchTime = currentMillis;

    // Only toggle to API screen if WiFi is actually up
    if (WiFi.status() == WL_CONNECTED) {
      showAPI = !showAPI;
      if (showAPI) fetchAPIWeather();
    } else {
      showAPI = false;  // stay on local if we're offline
    }
  }

  if (showAPI && WiFi.status() == WL_CONNECTED) {
    showAPIScreen();
  } else {
    showLocalScreen(temperature, humidity, pressure, lightLevel);
  }

  delay(1000);
}