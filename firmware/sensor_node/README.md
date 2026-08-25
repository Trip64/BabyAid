# Sensor Node Firmware (ESP32-C6)

The **Sensor Node** is a low-power wearable vital signs acquisition unit powered by the Espressif ESP32-C6 (RISC-V architecture). It continuously samples infant body temperature and photoplethysmography (PPG) waveforms, processes raw readings in dedicated FreeRTOS threads, performs on-device edge anomaly screening, and transmits real-time telemetry over Wi-Fi.

---

## Hardware Specifications

| Component | Function | Interface / Pin | Notes |
| :--- | :--- | :--- | :--- |
| **ESP32-C6** | Core Microcontroller | RISC-V 160MHz | 512KB SRAM, 4MB Flash, Wi-Fi 6 |
| **MAX30100 / MAX30102** | Pulse Oximeter & Heart Rate | I2C (SDA: GPIO 0, SCL: GPIO 1) | 660nm Red / 880nm IR optical sensing |
| **LM35 / NTC** | Skin Surface Temperature | ADC (GPIO 2) | Calibrated 10mV/°C linear analog output |

---

## Circuit & Wiring Specification

| Sensor / Module | Sensor Pin | ESP32-C6 GPIO | Protocol / Electrical Level | Notes |
|---|---|---|---|---|
| **MAX30100 / MAX30102** | `SDA` | `GPIO 0` | I2C Data (400 kHz) | Requires 4.7k $\Omega$ pull-up to 3.3V |
| **MAX30100 / MAX30102** | `SCL` | `GPIO 1` | I2C Clock (400 kHz) | Requires 4.7k $\Omega$ pull-up to 3.3V |
| **MAX30100 / MAX30102** | `VIN` / `GND` | `3V3` / `GND` | Power | 3.3V DC Regulated |
| **LM35 Temperature** | `VOUT` (Pin 2) | `GPIO 2` | 12-bit ADC Input | $10\text{ mV}/^\circ\text{C}$ linear analog voltage |
| **LM35 Temperature** | `VS` / `GND` | `3V3` / `GND` | Power | Connect bypass capacitor (100nF) |

```mermaid
flowchart LR
    subgraph MCU ["ESP32-C6 (RISC-V)"]
        G0["GPIO 0 (SDA)"]
        G1["GPIO 1 (SCL)"]
        G2["GPIO 2 (ADC)"]
    end

    subgraph PPG ["MAX30100/102 PPG"]
        SDA["SDA Line"]
        SCL["SCL Line"]
    end

    subgraph THERM ["LM35 Sensor"]
        VOUT["VOUT Analog"]
    end

    G0 <-->|I2C Data (4.7k Pullup)| SDA
    G1 -->|I2C Clock (4.7k Pullup)| SCL
    VOUT -->|10mV/°C Analog Voltage| G2
```

---

## Software Architecture

1. **FreeRTOS Asynchronous Sampling**: Optical PPG FIFO registers are polled every 1ms inside a dedicated FreeRTOS task (`poxUpdateTask`), isolating time-sensitive pulse timing from blocking network I/O.
2. **Signal Filtering**:
   - **LM35**: 16x hardware ADC oversampling with 12-bit resolution.
   - **MAX30100**: Heart rate outlier rejection (>25 BPM sudden jump cutoff) and Exponential Moving Average (EMA) smoothing:
     $$HR_{smoothed} = 0.90 \times HR_{prev} + 0.10 \times HR_{raw}$$
3. **Edge ML Anomaly Screener** (`ml_model.h`): On-device screening evaluates temperature bounds, bradycardia/tachycardia conditions, and heart rate variability (HRV) stability before network dispatch.
4. **Low Power Transmission**: RF output power is tuned to `8.5 dBm` to prevent transient LDO voltage droop and maximize battery runtime.

---

## Build & Flashing Instructions

1. **Install Dependencies** (Arduino IDE or PlatformIO):
   - `MAX30100lib` by OXullo Intersecans
   - `WiFi` & `HTTPClient` (Espressif ESP32 core >= 3.0.0)
2. **Configure Network**:
   ```bash
   cp config.example.h config.h
   ```
   Edit `config.h` to set your target Wi-Fi credentials or keep default access point pairing (`BabyMonitor` / `12345678`).
3. **Select Board**:
   - Board: `ESP32C6 Dev Module`
   - Flash Size: `4MB`
   - CPU Frequency: `160MHz`
4. **Flash**: Compile and upload to the ESP32-C6 via USB-C.
