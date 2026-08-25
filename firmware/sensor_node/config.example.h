#ifndef CONFIG_H
#define CONFIG_H

// ==============================================================================
// Sensor Node Configuration Template
// Rename this file to 'config.h' and configure your network parameters.
// ==============================================================================

// --- Hardware Pin Definitions (ESP32-C6 / RISC-V) ---
#define PIN_I2C_SDA          0
#define PIN_I2C_SCL          1
#define PIN_LM35_ADC         2

// --- Sampling & Calibration Constants ---
#define SEND_INTERVAL_MS     2000
#define ADC_OVERSAMPLES      16
#define ADC_BIT_RESOLUTION   12
#define ADC_VREF_MILLIVOLTS  3300

// --- Heart Rate Exponential Moving Average (EMA) Parameters ---
#define HR_MIN_VALID_BPM     50
#define HR_MAX_VALID_BPM     170
#define HR_OUTLIER_THRESHOLD 25
#define HR_EMA_ALPHA         0.10f // Weight of new sample (0.10 new, 0.90 history)

// --- Wireless Network Credentials (Display Node SoftAP) ---
#define WIFI_SSID            "BabyMonitor"
#define WIFI_PASSWORD        "12345678"
#define SERVER_POST_URL      "http://192.168.4.1/data"
#define WIFI_CONNECT_TIMEOUT 15000 // Milliseconds

#endif // CONFIG_H
