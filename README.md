# Weather_station

# ESP8266 Weather Station

A compact dual-mode weather station built on the ESP8266 (NodeMCU/Wemos D1 Mini) that displays real-time sensor readings alongside live OpenWeatherMap data on a 128×64 OLED screen. If WiFi is unavailable at startup, it falls back to local sensor readings and keeps retrying in the background.

---

## Features

- **Local sensor readings** — temperature (DHT11), humidity (DHT11), barometric pressure (BMP180), and ambient light (BH1750)
- **OpenWeatherMap API** — fetches live outdoor temperature, humidity, pressure, and weather condition
- **Auto-alternating screens** — switches between local and API data every 30 seconds with a live countdown timer
- **10-second WiFi startup timeout** — shows a progress bar on the OLED; falls back to local-only mode if no connection is made
- **Background WiFi reconnection** — retries every 30 seconds while running; seamlessly enables the API screen once connected
- **WiFi status indicator** — local screen shows `[W]` when WiFi is active

---

## Hardware

| Component | Part | Interface |
|---|---|---|
| Microcontroller | ESP8266 (NodeMCU v3 / Wemos D1 Mini) | — |
| Display | SSD1306 128×64 OLED | I²C (0x3C) |
| Temp/Humidity | DHT11 | Digital (D4) |
| Barometric pressure | BMP180 | I²C |
| Ambient light | BH1750 | I²C |

### Wiring

```
DHT11 data  →  D4
SDA         →  D2  (shared I²C bus: OLED + BMP180 + BH1750)
SCL         →  D1
3.3V / GND  →  all sensors
```

---

## Software Dependencies

Install all libraries via **Arduino IDE → Library Manager**:

| Library | Author |
|---|---|
| `Adafruit SSD1306` | Adafruit |
| `Adafruit GFX Library` | Adafruit |
| `DHT sensor library` | Adafruit |
| `Adafruit BMP085 Unified` | Adafruit |
| `BH1750` | Christopher Laws |
| `ESP8266WiFi` | ESP8266 Community (bundled with board package) |

**Board package:** Install `esp8266` by ESP8266 Community via **File → Preferences → Additional Board Manager URLs:**
```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

---

## Configuration

Create a `secrets.h` file in the same folder as the `.ino` file:

```cpp
#define HOME_WIFI_SSID       "your_wifi_ssid"
#define HOME_WIFI_PASSWORD   "your_wifi_password"
#define OPENWEATHER_API_KEY  "your_api_key_here"
```

Get a free API key at [openweathermap.org/api_keys](https://home.openweathermap.org/api_keys).

The city and country are set in the main sketch:

```cpp
const char* city        = "Ballito";
const char* countryCode = "ZA";
```

---

## Upload Settings

| Setting | Value |
|---|---|
| Board | NodeMCU 1.0 (ESP-12E Module) |
| Upload Speed | 115200 |
| Flash Size | 4MB (FS:2MB OTA:~1019KB) |
| Port | COM port shown in Device Manager |

> **Tip:** If you get a `Write timeout` error, hold the **FLASH/IO0** button on the board while clicking Upload, then release once `Connecting...` appears.

---

## How It Works

### Startup sequence

1. OLED, DHT11, BMP180, and BH1750 are initialised
2. WiFi connection is attempted with a **10-second countdown** shown on the OLED as a progress bar
3. **Connected** → fetches API weather data, enters normal loop
4. **Not connected** → shows a "local only" message, enters loop without API screen

### Main loop

```
Every 1 second   →  Read all sensors, refresh current screen
Every 30 seconds →  Switch between local and API screen (API only if WiFi is up)
Every 30 seconds →  If WiFi is down, attempt reconnection (5s window)
```

Once WiFi reconnects mid-session, the API screen becomes available automatically on the next 30-second switch.

### Screen layout

**Local screen**
```
Local Wx [W]               27s
Temp: 22.50 C
Hum:  61.00 %
Pres: 1013.25 hPa
Light:342 lx
```

**API screen**
```
API Weather                12s
Temp: 25.3 C
Hum:  70 %
Pres: 1011 hPa
Cond:clear sky
```

The number in the top-right corner counts down to the next screen switch.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Write timeout` on upload | Hold FLASH button during upload; try 115200 baud; check USB cable is data-capable |
| OLED stays blank | Confirm I²C address is `0x3C`; check SDA/SCL wiring |
| `BMP180 failed` on screen | Check I²C wiring; confirm 3.3V supply (not 5V) |
| Temperature reads high | Increase the `- 5` offset in `dht.readTemperature() - 5` to compensate for self-heating |
| API shows `--` / `WiFi fail` | Check `secrets.h` credentials; verify API key is active (takes ~10 min after creation) |
| API shows `Parse fail` | OpenWeatherMap response format changed; print `payload` to Serial Monitor to debug |

---

## Project Structure

```
weather-station/
├── weather-station.ino   # Main sketch
└── secrets.h             # WiFi credentials and API key (not committed to git)
```

> **Never commit `secrets.h` to a public repository.** Add it to `.gitignore`:
> ```
> secrets.h
> ```

---

## License

MIT
