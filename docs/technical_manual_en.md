# KIDSENTINEL: Technical Design & Algorithm Analysis Manual

## 1. Introduction & Mission

KIDSENTINEL is an advanced medical-grade Internet of Things (IoT) infant vital sign monitoring platform. By combining modern 32-bit RISC-V and Xtensa microcontrollers with optical biosensors and precision environmental transducers, it brings continuous non-invasive vital sign monitoring from clinical intensive care units (NICUs) into the domestic nursery environment.

---

## 2. Hardware Architecture & Transducers

### 2.1. ESP32-C6 (Wearable Sensor Node)
- **Core**: 32-bit RISC-V CPU operating at 160 MHz.
- **Memory**: 512 KB SRAM, 4 MB Quad SPI Flash.
- **RF Subsystem**: Wi-Fi 6 (802.11ax), Bluetooth 5 (LE), IEEE 802.15.4 (Zigbee/Thread).
- **Bus Speed**: Hardware I2C operating at 400 kHz on GPIO 0 (SDA) and GPIO 1 (SCL).

### 2.2. MAX30100 / MAX30102 Optical PPG Sensor
- **Emitter LEDs**: 660 nm (Red) and 880 nm (Infrared).
- **Integrated ADC**: 16-bit Sigma-Delta ADC with up to 1000 samples per second.
- **FIFO Buffer**: 16-sample deep FIFO preventing data loss during burst tasks.

### 2.3. SHT31 Ambient Environmental Transducer
- **Temperature Accuracy**: $\pm 0.2^\circ\text{C}$ over $0^\circ\text{C}$ to $90^\circ\text{C}$.
- **Relative Humidity Accuracy**: $\pm 2\%$ RH over $0\%$ to $100\%$ RH.
- **Thermal Neutral Zone (TNZ) Monitoring**: Ensures infant sleeping room remains within safe biological equilibrium.

---

## 3. Signal Processing & Optical Theory

### 3.1. Photoplethysmography (PPG) & The Beer-Lambert Law

Photoplethysmography measures volumetric blood changes in microvascular tissue beds. The transmission of optical energy through biological tissue is governed by the modified **Beer-Lambert Law**:

$$I = I_0 \cdot \exp\left(-(\mu_a + \mu_s) \cdot d\right)$$

Where:
- $I_0$: Incident light intensity.
- $I$: Transmitted/reflected light intensity detected by photodiode.
- $\mu_a$: Absorption coefficient (dependent on oxyhemoglobin $\text{HbO}_2$ and deoxyhemoglobin $\text{Hb}$ concentrations).
- $\mu_s$: Scattering coefficient of dermal tissue.
- $d$: Optical path length.

### 3.2. Heart Rate Extraction Pipeline

1. **Anti-aliasing and DC Removal**: High-pass filtering removes low-frequency baseline wander caused by respiration and slow motion.
2. **Exponential Moving Average (EMA)**:
   $$y[n] = \alpha \cdot x[n] + (1 - \alpha) \cdot y[n-1]$$
   With smoothing coefficient $\alpha = 0.10$, rejecting high-frequency electromechanical noise.
3. **Adaptive Threshold Peak Detection**: Evaluates derivative zero-crossings to measure Inter-Beat Intervals (IBI or $T_{\text{RR}}$ in milliseconds).
4. **Heart Rate Calculation**:
   $$\text{BPM} = \frac{60\,000}{T_{\text{RR}}}$$

---

## 4. HMI Graphics Engine & Alerting

The CrowPanel unit leverages **LovyanGFX** for flicker-free double-buffered rendering on the ILI9341 SPI controller.

- **Color Coding**: High-contrast dark theme optimized for low-light nursery visibility.
- **Dynamic Alarms**: Non-blocking timer loops monitor vital limits ($T_{\text{infant}} > 38.0^\circ\text{C}$, $\text{HR} < 50$ BPM, $\text{HR} > 160$ BPM) and actuate the hardware piezo buzzer via pulse-width modulation.

---

## 5. References

1. Maxim Integrated (2020), *MAX30102 High-Sensitivity Pulse Oximeter Datasheet*.
2. Espressif Systems (2024), *ESP32-C6 Series Technical Reference Manual*.
3. Webster, J. G., *Design of Pulse Oximeters*, Taylor & Francis.
4. World Health Organization (WHO), *Technical Specifications for Neonatal Monitoring Devices*.
