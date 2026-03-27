#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DHTPIN D4
#define DHTTYPE DHT11

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    for (;;);
  }

  if (!bmp.begin()) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("BMP180 failed");
    display.display();
    while (1);
  }

  lightMeter.begin();

  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature() -5;
  float pressure = bmp.readPressure() / 100.0;
  float lightLevel = lightMeter.readLightLevel();


  display.clearDisplay();
  display.setTextSize(1);

  display.setTextSize(1);   // BIG text
  display.setCursor(0, 0);
  display.println("Weather Station");

  display.setTextSize(1);   // back to normal

  display.setCursor(0, 17);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");


  display.setCursor(0, 27);
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");

  display.setCursor(0, 37);
  display.print("Press: ");
  display.print(pressure);
  display.println(" hPa");

  display.setCursor(0, 47);
  display.print("Light: ");
  display.print(lightLevel);
  display.println(" lx");

  display.display();

  delay(2000);
}