<div align="center">

# BabyAid

### *Infant Vital Signs & Environmental Monitor with ML*

[![Architecture: Distributed IoT](https://img.shields.io/badge/Architecture-Distributed%20Biomedical%20IoT-00A9CE.svg)](#system-architecture)
[![Core: RISC-V & Xtensa](https://img.shields.io/badge/Architecture-RISC--V%20%7C%20Xtensa-3E84F6.svg)](#hardware-pinouts)
[![RTOS: FreeRTOS Deterministic](https://img.shields.io/badge/RTOS-FreeRTOS%20Real--Time-008080.svg)](#key-features)
[![Edge ML: TinyML Random Forest](https://img.shields.io/badge/Edge%20ML-TinyML%20%2F%20Scikit--Learn-F7931E.svg)](#machine-learning--tinyml-pipeline)
[![UI: LovyanGFX Color TFT](https://img.shields.io/badge/UI-LovyanGFX%20320x240%20IPS-9B51E0.svg)](#hardware-pinouts)
[![License: MIT](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

<p align="center">
  <b>Wearable PPG pulse sensor + body thermometer + room environment monitor, with on-device anomaly detection and a standalone web dashboard. No cloud needed.</b>
</p>

[TL;DR](#tldr) •
[System Architecture](#system-architecture) •
[Machine Learning Pipeline](#machine-learning--tinyml-pipeline) •
[Hardware Pinouts](#hardware-pinouts) •
[Project Roadmap](#project-roadmap--todo) •
[Clinical Documentation](docs/)

---

</div>

## TL;DR

- **Continuous infant monitoring**: Instead of a passive audio baby monitor, this continuously tracks skin temperature (LM35) and heart rate via optical PPG (MAX30100/MAX30102).
- **ML anomaly detection on the MCU**: A TinyML Random Forest runs directly on the sensor node. It flags bradycardia, tachycardia, fever trends ($dT/dt$), and HRV drops before sending an alert.
- **Reliable PPG sampling**: A dedicated FreeRTOS task polls the optical FIFO every 1ms so heartbeat peaks don't get lost when the Wi-Fi stack is busy.
- **Infant vs. room temperature**: Compares the baby's skin temp against the room's SHT31 reading ($\Delta T = T_{\text{infant}} - T_{\text{ambient}}$) to catch environmental problems too.
- **Works offline**: The display station runs its own Wi-Fi access point and web server. Connect any phone or laptop directly — no internet, no subscription, no cloud account.

---

## System Architecture

```mermaid
flowchart TD
    subgraph Sensor_Tier ["Wearable Sensor Node (RISC-V @ 160MHz)"]
        PPG["MAX30100 / MAX30102<br/>Optical PPG Sensor (I2C 0x57)"]
        TEMP["LM35 Body Temperature<br/>16x ADC Oversampling (GPIO 2)"]
        RTOS["FreeRTOS Engine<br/>1ms FIFO Poll & EMA Smoothing"]
        ML["TinyML Anomaly Screener<br/>On-Device Decision Trees"]
        
        PPG --> RTOS
        TEMP --> RTOS
        RTOS --> ML
    end

    subgraph Base_Tier ["Display Node & Access Point Hub (CrowPanel 2.4-inch)"]
        AP["SoftAP Access Point<br/>(SSID: BabyMonitor @ 192.168.4.1)"]
        SHT["Sensirion SHT31<br/>Ambient Temp & Humidity (I2C 0x44)"]
        GFX["LovyanGFX Engine<br/>320x240 Color Dashboard"]
        AUDIO["Tone PWM Engine<br/>Non-blocking Piezo Alarm"]
        SERVER["Embedded REST Server<br/>Port 80 JSON State Engine"]
        
        SHT --> SERVER
        SERVER --> GFX
        SERVER --> AUDIO
    end

    subgraph Presentation_Tier ["Client Presentation & Cloud"]
        DASH["Responsive Dark-Mode Web App<br/>(Mobile / Tablet / Desktop)"]
        PY_ML["Python ML Pipeline<br/>(HRV Ingestion & Retraining)"]
    end

    ML -- "HTTP POST /data (8.5 dBm Low-Power STA)" --> SERVER
    SERVER -- "HTTP GET /api/state (JSON)" --> DASH
    SERVER -- "Live Telemetry Stream" --> PY_ML
```

---

## Key Features

- **1ms PPG sampling**: Dedicated FreeRTOS task drains the optical FIFO continuously so heartbeats don't get lost during network traffic.
- **Signal filtering**: Rejects outlier jumps (>25 BPM) and applies EMA smoothing for stable heart rate readings.
- **On-device ML screening**: Checks temperature bounds, heart rate anomalies, and HRV stability right on the sensor node before sending anything over the network.
- **Room + body temperature**: Reads both the infant's skin temp and the nursery ambient temp/humidity (SHT31) simultaneously.
- **Self-contained network**: Display station creates its own Wi-Fi AP and serves the dashboard. No router needed.
- **Non-blocking alarms**: Visual banners, heart animation, and piezo buzzer with a hardware mute button.

---

## Machine Learning & TinyML Pipeline

The ML side is a Python pipeline that takes raw telemetry, engineers clinical features, trains classifiers, and compiles them into a C++ header you can flash directly onto the MCU.

```mermaid
flowchart LR
    A["Raw Telemetry Stream<br/>(PPG & Temperature)"] --> B["data_collector.py<br/>(CSV Logging & Labeling)"]
    B --> C["features.py<br/>HRV RMSSD, SDNN, pNN50<br/>Thermal Velocity (dT/dt)"]
    C --> D["train_model.py<br/>Random Forest Classifier<br/>Isolation Forest Detector"]
    D --> E["export_tinyml.py<br/>Zero-Dependency C++ Compiler"]
    E --> F["ml_model.h<br/>On-Device Microcontroller Inference"]
```

### Feature Engineering & Clinical Indices
- `body_temp`: Infant skin/core temperature (°C) for hyperthermia and neonatal hypothermia detection.
- `heart_rate`: Smoothed optical pulse rate (BPM) for tachycardia and bradycardia screening.
- `hrv_rmssd`: Root Mean Square of Successive RR Differences (ms) measuring parasympathetic autonomic tone.
- `hrv_sdnn`: Standard deviation of RR intervals (ms) assessing overall cardiac autonomic regulation.
- `temp_slope`: Rate of temperature change ($^\circ\text{C}/\text{min}$) detecting rapid fever onset.
- `temp_room_delta`: Infant-to-ambient temperature differential ($\Delta T$) evaluating environmental thermal stress.

---

## Hardware Pinouts

### 1. Sensor Node (ESP32-C6 Dev Module)

| Peripheral | Pin / GPIO | Electrical Protocol | Description |
|---|---|---|---|
| **I2C SDA** | `GPIO 0` | I2C Data (400 kHz) | MAX30100 / MAX30102 PPG data line (4.7k $\Omega$ pull-up) |
| **I2C SCL** | `GPIO 1` | I2C Clock (400 kHz) | MAX30100 / MAX30102 PPG clock line (4.7k $\Omega$ pull-up) |
| **LM35 ADC** | `GPIO 2` | 12-bit Analog In | Analog body temperature sensor ($10\text{ mV}/^\circ\text{C}$) |
| **Power In** | `3V3` & `GND` | Power | 3.3V DC Regulated supply |

### 2. Display Node (CrowPanel 2.4" ILI9341 ESP32)

| Peripheral | Pin / GPIO | Electrical Protocol | Description |
|---|---|---|---|
| **TFT SPI SCLK** | `GPIO 14` | SPI Clock (40 MHz) | ILI9341 display clock |
| **TFT SPI MOSI** | `GPIO 13` | SPI Data In | ILI9341 master out slave in |
| **TFT SPI MISO** | `GPIO 12` | SPI Data Out | ILI9341 master in slave out |
| **TFT SPI D/C** | `GPIO 2` | Digital Control | Data / Command selector line |
| **TFT SPI CS** | `GPIO 15` | Digital Output | Active LOW chip select |
| **TFT Backlight** | `GPIO 27` | Digital Output | Active HIGH backlight enable |
| **Piezo Buzzer** | `GPIO 26` | PWM / Tone | Frequency-modulated audio alarm |
| **Boot Button** | `GPIO 0` | Digital In (`INPUT_PULLUP`) | Hardware alarm mute toggle |
| **SHT31 SDA / SCL** | `GPIO 21` / `GPIO 22` | I2C Fast Mode (400 kHz) | Nursery ambient temperature & humidity |

---

## Project Roadmap & TODO

- [ ] **Low-power BLE mode**: Switch from Wi-Fi to connectionless BLE advertising to get current draw under $< 2\text{mA}$.
- [ ] **SpO2 calibration**: Finish the dual-wavelength lookup table for blood oxygen percentage.
- [ ] **Port to nRF54L15**: Move the sensor firmware and ML inference to Cortex-M33 for better battery life.
- [ ] **Sensor detachment detection**: Add capacitive lead-off sensing so it knows when the sensor falls off vs. when the baby moves.
- [ ] **Optional cloud sync**: Add MQTT / Home Assistant bridging for long-term logging if someone wants it.

---

## Repository Structure

- [`docs/`](docs/) — Detailed clinical rationale and technical manuals:
  - [`architecture.md`](docs/architecture.md) — System topology, bus specifications, and REST API routes
  - [`technical_manual_en.md`](docs/technical_manual_en.md) — Signal processing theory and Beer-Lambert equations
  - [`clinical_findings_en.md`](docs/clinical_findings_en.md) — Clinical background on SIDS, bradycardia, and thermoregulation
- [`firmware/`](firmware/) — Microcontroller source code:
  - [`sensor_node/`](firmware/sensor_node/) — Wearable sensor acquisition, FreeRTOS tasks, and TinyML model
  - [`display_node/`](firmware/display_node/) — Station display manager, LovyanGFX UI, and embedded web server
- [`ml/`](ml/) — Python machine learning suite:
  - [`train_model.py`](ml/train_model.py) — Random Forest & Isolation Forest training
  - [`features.py`](ml/features.py) — HRV & thermal feature extractor
  - [`export_tinyml.py`](ml/export_tinyml.py) — C++ header model compiler
- [`web/`](web/) — Standalone interactive web dashboard preview
- [`tools/`](tools/) — Universal I2C bus diagnostic scanner

---

## Getting Started

### 1. Flash the Display Node
1. Open [`firmware/display_node/display_node.ino`](firmware/display_node/display_node.ino) in Arduino IDE or PlatformIO.
2. Install `LovyanGFX` and `Adafruit SHT31` libraries.
3. Select board `ESP32 Dev Module` and flash to the CrowPanel device.

### 2. Flash the Sensor Node
1. Open [`firmware/sensor_node/sensor_node.ino`](firmware/sensor_node/sensor_node.ino).
2. Install `MAX30100lib`.
3. Select board `ESP32C6 Dev Module` and flash.

### 3. Connect to the Live Dashboard
1. Connect your smartphone or laptop to the Wi-Fi network:
   - **SSID**: `BabyMonitor`
   - **Password**: `12345678`
2. Open your browser and navigate to:
   ```
   http://192.168.4.1/
   ```

---

## License

This project is open-source and released under the [MIT License](LICENSE).
