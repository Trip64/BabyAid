# Display Node & Access Point Hub Firmware

The **Display Node** acts as the central station and human-machine interface (HMI) for the Kidsentinel system. Hosted on an ESP32 CrowPanel 2.4" TFT device, it acts as a standalone Wi-Fi SoftAP access point, runs an embedded HTTP REST and web server, reads ambient nursery conditions via an SHT31 sensor, renders a color-coded vital signs dashboard using **LovyanGFX**, and manages non-blocking audio/visual alarms.

---

## Hardware Specifications

| Component | Function | Pins / Interface | Description |
| :--- | :--- | :--- | :--- |
| **CrowPanel 2.4"** | Central Controller & Screen | ESP32-WROOM-32 | 320x240 ILI9341 Color TFT via SPI |
| **SHT31-D** | Ambient Temp & Humidity | I2C (SDA: 21, SCL: 22) | High-precision nursery environmental sensing |
| **Piezo Speaker** | Critical Audio Alarms | GPIO 26 | Non-blocking PWM tone generation |
| **Backlight Control** | TFT Backlight Power | GPIO 27 | Active HIGH enable |
| **Boot Button** | Alarm Mute / Unmute | GPIO 0 | Hardware debounce toggle |

---

## Hardware Pinout Map

| ESP32 Pin / GPIO | Direction | Connected Peripheral | Electrical Function |
|---|---|---|---|
| `GPIO 14` | Output | ILI9341 LCD | SPI Serial Clock (40 MHz) |
| `GPIO 13` | Output | ILI9341 LCD | SPI Master Out Slave In |
| `GPIO 12` | Input | ILI9341 LCD | SPI Master In Slave Out |
| `GPIO 2` | Output | ILI9341 LCD | Data / Command Selection |
| `GPIO 15` | Output | ILI9341 LCD | Chip Select (Active LOW) |
| `GPIO 27` | Output | Display Backlight | Active HIGH Backlight Enable |
| `GPIO 26` | Output | Piezo Buzzer | PWM Frequency-Modulated Tone Alert |
| `GPIO 0` | Input | Onboard BOOT Button | Hardware Alarm Mute Toggle (`INPUT_PULLUP`) |
| `GPIO 21` | Bidirectional | Sensirion SHT31 | I2C Data (`0x44`, 400 kHz) |
| `GPIO 22` | Output | Sensirion SHT31 | I2C Clock (`0x44`, 400 kHz) |

---

## Web API Endpoints

The node broadcasts a standalone Wi-Fi network `BabyMonitor` (Default IP: `192.168.4.1`) and exposes:

| Method | Route | Description | Parameters / Payload |
| :--- | :--- | :--- | :--- |
| `GET` | `/` | Responsive dark-mode web dashboard | Returns `INDEX_HTML` |
| `GET` | `/api/state` | Real-time system state (JSON) | `{"bt":36.5, "hr":112, "rt":22.4, "rh":50, "alert":false}` |
| `POST` | `/api/settings` | Modify runtime calibration & alarm mute | Form urlencoded: `trim=4.0&mute=1` |
| `POST` | `/data` | Ingest sensor node telemetry | Form urlencoded: `temp=32.5&ir=112` |

---

## Build & Flashing Instructions

1. **Install Required Libraries**:
   - `LovyanGFX` by lovyan03 (v1.1.x+)
   - `Adafruit SHT31 Library` by Adafruit
   - `WiFi` & `WebServer` (ESP32 Arduino Core)
2. **Configure Settings**:
   ```bash
   cp config.example.h config.h
   ```
3. **Select Board in Arduino IDE / PlatformIO**:
   - Board: `ESP32 Dev Module`
   - Flash Size: `4MB`
   - Partition Scheme: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)`
4. **Flash**: Upload sketch to the CrowPanel device.
