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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
BH1750 lightMeter;

// ---------------- WIFI + API ----------------
const char* ssid = HOME_WIFI_SSID;
const char* password = HOME_WIFI_PASSWORD;
const char* apiKey = OPENWEATHER_API_KEY; //https://home.openweathermap.org/api_keys
const char* city = "Ballito";
const char* countryCode = "ZA";

// ---------------- TIMING ----------------
unsigned long lastSwitchTime = 0;
const unsigned long switchInterval = 30000; // 30 seconds

bool showAPI = false;

// ---------------- API DATA ----------------
String apiTemp = "--";
String apiHumidity = "--";
String apiPressure = "--";
String apiCondition = "No data";

// ---------------- HELPERS ----------------
String extractJsonValue(String json, String key) {
  int keyIndex = json.indexOf(key);
  if (keyIndex == -1) return "--";

  int colonIndex = json.indexOf(':', keyIndex);
  if (colonIndex == -1) return "--";

  int startIndex = colonIndex + 1;

  while (startIndex < json.length() &&
         (json[startIndex] == ' ' || json[startIndex] == '\"')) {
    startIndex++;
  }

  int endIndex = startIndex;

  while (endIndex < json.length() &&
         json[endIndex] != ',' &&
         json[endIndex] != '}' &&
         json[endIndex] != '\"') {
    endIndex++;
  }

  return json.substring(startIndex, endIndex);
}

bool fetchAPIWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    apiTemp = "--";
    apiHumidity = "--";
    apiPressure = "--";
    apiCondition = "WiFi fail";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://api.openweathermap.org/data/2.5/weather?q=" +
               String(city) + "," + String(countryCode) +
               "&appid=" + String(apiKey) +
               "&units=metric";

  Serial.println("========== API REQUEST ==========");
  Serial.println("URL:");
  Serial.println(url);

  http.begin(client, url);
  int httpCode = http.GET();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String payload = http.getString();

    Serial.println("Payload:");
    Serial.println(payload);

    if (httpCode == 200) {
      apiTemp = extractJsonValue(payload, "\"temp\"");
      apiHumidity = extractJsonValue(payload, "\"humidity\"");
      apiPressure = extractJsonValue(payload, "\"pressure\"");
      apiCondition = extractJsonValue(payload, "\"description\"");

      if (apiTemp == "--" || apiHumidity == "--" || apiPressure == "--") {
        Serial.println("Parse failed");
        apiTemp = "--";
        apiHumidity = "--";
        apiPressure = "--";
        apiCondition = "Parse fail";
        http.end();
        return false;
      }

      http.end();
      return true;
    } else {
      Serial.println("API returned error code (not 200)");
      apiTemp = "--";
      apiHumidity = "--";
      apiPressure = "--";
      apiCondition = "API fail";
      http.end();
      return false;
    }
  } else {
    Serial.print("HTTP error: ");
    Serial.println(http.errorToString(httpCode));

    apiTemp = "--";
    apiHumidity = "--";
    apiPressure = "--";
    apiCondition = "Conn fail";

    http.end();
    return false;
  }
}

void drawCountdown() {
  unsigned long elapsed = millis() - lastSwitchTime;
  unsigned long remaining = 0;

  if (elapsed < switchInterval) {
    remaining = (switchInterval - elapsed) / 1000;
  }

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
  display.println("Local Weather");

  drawCountdown();

  display.setCursor(0, 16);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.setCursor(0, 26);
  display.print("Hum:  ");
  display.print(humidity);
  display.println(" %");

  display.setCursor(0, 36);
  display.print("Pres: ");
  display.print(pressure);
  display.println(" hPa");

  display.setCursor(0, 46);
  display.print("Light:");
  display.print(lightLevel, 0);
  display.println(" lx");

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
  display.print("Temp: ");
  display.print(apiTemp);
  display.println(" C");

  display.setCursor(0, 26);
  display.print("Hum:  ");
  display.print(apiHumidity);
  display.println(" %");

  display.setCursor(0, 36);
  display.print("Pres: ");
  display.print(apiPressure);
  display.println(" hPa");

  display.setCursor(0, 46);
  display.print("Cond:");
  display.println(apiCondition);

  display.display();
}

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

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());

  fetchAPIWeather();
  lastSwitchTime = millis();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature() - 5;
  float pressure = bmp.readPressure() / 100.0;
  float lightLevel = lightMeter.readLightLevel();

  unsigned long currentMillis = millis();

  if (currentMillis - lastSwitchTime >= switchInterval) {
    lastSwitchTime = currentMillis;
    showAPI = !showAPI;

    if (showAPI) {
      fetchAPIWeather();
    }
  }

  if (showAPI) {
    showAPIScreen();
  } else {
    showLocalScreen(temperature, humidity, pressure, lightLevel);
  }

  delay(1000);
}