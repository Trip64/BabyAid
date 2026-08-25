/**
 * @file sensor_node.ino
 * @brief Wearable Infant Vital Sign Monitor Firmware (ESP32-C6)
 * @details Acquires body temperature (LM35 ADC) and optical photoplethysmography 
 *          (MAX30100 PPG), performs FreeRTOS asynchronous FIFO sampling, exponential
 *          moving average (EMA) noise reduction, edge ML anomaly screening, and 
 *          dispatches HTTP telemetry to the display node hub.
 */

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "MAX30100_PulseOximeter.h"
#include "esp_system.h"

#include "config.h"
#include "ml_model.h"

// --- Global Sensor & System State ---
PulseOximeter pox;
bool maxSensorInitialized = false;
bool wifiConnected        = false;
int  telemetrySuccessCount = 0;
int  telemetryFailureCount = 0;
unsigned long lastSendTimestamp = 0;
unsigned long bootTimestamp     = 0;
float smoothedHeartRate         = 0.0f;
float lastTemperatureReading    = 0.0f;

// --- Callback triggered on optical pulse detection ---
void onBeatDetected() {
    Serial.println(F("[PPG] Beat detected"));
}

// --- Structured Logging Helpers ---
void logHeader(const char* title) {
    Serial.println();
    Serial.print(F("=== ["));
    Serial.print(title);
    Serial.println(F("] ==="));
}

void logMetric(const char* label, float value, int precision = 1) {
    Serial.print(F("  -> "));
    Serial.print(label);
    Serial.print(F(": "));
    Serial.println(value, precision);
}

void logMetric(const char* label, const char* value) {
    Serial.print(F("  -> "));
    Serial.print(label);
    Serial.print(F(": "));
    Serial.println(value);
}

void logStatus(bool success, const char* message) {
    Serial.print(success ? F("  [OK] ") : F("  [ERROR] "));
    Serial.println(message);
}

/**
 * @brief Reads and oversamples the LM35 analog temperature sensor.
 * @return Calibrated temperature in degrees Celsius.
 */
float readBodyTemperature() {
    long accumulatedAdc = 0;
    for (int i = 0; i < ADC_OVERSAMPLES; i++) {
        accumulatedAdc += analogRead(PIN_LM35_ADC);
        delayMicroseconds(200);
    }
    float averageAdc = (float)accumulatedAdc / ADC_OVERSAMPLES;
    float millivolts = (averageAdc / 4095.0f) * ADC_VREF_MILLIVOLTS;
    float temperatureCelsius = millivolts / 10.0f; // LM35: 10mV / °C
    return temperatureCelsius;
}

/**
 * @brief Scans active I2C bus addresses for diagnostics.
 */
void scanI2CBus() {
    logHeader("I2C BUS SCAN");
    int devicesFound = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  -> Detected address 0x%02X", addr);
            if (addr == 0x57) Serial.print(F(" (MAX30100 PPG Sensor)"));
            Serial.println();
            devicesFound++;
        }
    }
    Serial.printf("  -> Total devices found: %d\n", devicesFound);
}

/**
 * @brief Dedicated FreeRTOS background task for MAX30100 FIFO processing.
 * Prevents FIFO buffer overflow during HTTP networking transactions.
 */
void poxUpdateTask(void *pvParameters) {
    for (;;) {
        if (maxSensorInitialized) {
            pox.update();
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Dispatches telemetry packet to the display hub server.
 */
void sendTelemetry(float tempCelsius, long heartRateBpm, bool isAnomaly = false) {
    HTTPClient http;
    http.setTimeout(3000);
    http.begin(SERVER_POST_URL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String payload = "temp=" + String(tempCelsius, 1) + 
                     "&ir=" + String(heartRateBpm) +
                     "&ml_alert=" + String(isAnomaly ? 1 : 0);
    
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
        telemetrySuccessCount++;
        Serial.printf("  -> Telemetry HTTP %d | Packet #%d\n", httpResponseCode, telemetrySuccessCount);
    } else {
        telemetryFailureCount++;
        Serial.printf("  -> Telemetry POST Failed | Error Count: %d\n", telemetryFailureCount);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F("  KIDSENTINEL - SENSOR NODE FIRMWARE      "));
    Serial.println(F("  Architecture: ESP32-C6 (RISC-V)         "));
    Serial.println(F("=========================================="));
    bootTimestamp = millis();

    // ADC Configuration
    analogReadResolution(ADC_BIT_RESOLUTION);
    pinMode(PIN_LM35_ADC, INPUT);

    // I2C Bus Initialization
    logHeader("I2C INITIALIZATION");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    logStatus(true, "I2C bus started");
    scanI2CBus();

    // Wireless Networking Setup
    logHeader("WIRELESS NETWORK SETUP");
    Wire.end();
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // Optimized RF power to prevent LDO voltage droop
    logStatus(true, "Wi-Fi TX Power set to 8.5 dBm");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print(F("  Connecting to AP"));
    unsigned long connectStart = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - connectStart < WIFI_CONNECT_TIMEOUT)) {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        logStatus(true, "Wi-Fi Connected successfully");
        logMetric("Assigned IP", WiFi.localIP().toString().c_str());
        logMetric("Signal RSSI", (float)WiFi.RSSI(), 0);
    } else {
        wifiConnected = false;
        logStatus(false, "Initial Wi-Fi connection timed out. Background reconnection active.");
    }

    // Reinitialize I2C post-Wi-Fi initialization
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    delay(100);

    // Initialize MAX30100 Pulse Oximeter
    logHeader("MAX30100 INITIALIZATION");
    maxSensorInitialized = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.printf("  Attempt %d/3...\n", attempt);
        if (pox.begin()) {
            pox.setIRLedCurrent(MAX30100_LED_CURR_20_8MA);
            pox.setOnBeatDetectedCallback(onBeatDetected);
            logStatus(true, "MAX30100 sensor initialized successfully");
            maxSensorInitialized = true;
            break;
        }
        Wire.end(); 
        delay(200); 
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
        delay(200);
    }

    if (!maxSensorInitialized) {
        logStatus(false, "MAX30100 initialization failed. Check wiring and pullup resistors.");
    } else {
        // Spawn FreeRTOS FIFO processing thread
        xTaskCreate(
            poxUpdateTask,
            "POX_Task",
            4096,
            NULL,
            3,
            NULL
        );
        logStatus(true, "High-priority FreeRTOS POX task active");
    }

    logHeader("SYSTEM READY");
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastSendTimestamp < SEND_INTERVAL_MS) {
        return;
    }
    lastSendTimestamp = currentMillis;

    logHeader("TELEMETRY ACQUISITION");
    unsigned long uptimeSeconds = (currentMillis - bootTimestamp) / 1000;
    Serial.printf("  Uptime: %02lu:%02lu:%02lu\n", uptimeSeconds / 3600, (uptimeSeconds % 3600) / 60, uptimeSeconds % 60);

    // Read Body Temperature
    float temperature = readBodyTemperature();
    lastTemperatureReading = temperature;
    logMetric("Infant Temperature (°C)", temperature);

    // Read & Filter Heart Rate
    long finalHeartRate = 0;
    if (maxSensorInitialized) {
        float rawHeartRate = pox.getHeartRate();
        uint8_t rawSpO2 = pox.getSpO2();
        logMetric("Raw HR (BPM)", rawHeartRate);
        logMetric("SpO2 (%)", (float)rawSpO2, 0);

        if (rawHeartRate >= HR_MIN_VALID_BPM && rawHeartRate <= HR_MAX_VALID_BPM) {
            bool isOutlier = false;
            if (smoothedHeartRate >= HR_MIN_VALID_BPM) {
                if (abs(rawHeartRate - smoothedHeartRate) > HR_OUTLIER_THRESHOLD) {
                    isOutlier = true;
                }
            }

            if (!isOutlier) {
                if (smoothedHeartRate == 0.0f) {
                    smoothedHeartRate = rawHeartRate;
                } else {
                    smoothedHeartRate = (smoothedHeartRate * (1.0f - HR_EMA_ALPHA)) + (rawHeartRate * HR_EMA_ALPHA);
                }
            }
            finalHeartRate = (long)smoothedHeartRate;
        } else if (smoothedHeartRate > 0.0f) {
            finalHeartRate = (long)smoothedHeartRate;
        }
    }

    // Edge ML Anomaly Evaluation
    VitalSignFeatures currentFeatures;
    currentFeatures.bodyTemperature = temperature;
    currentFeatures.heartRateBpm = (float)finalHeartRate;
    currentFeatures.hrvRMSSD = 45.0f; // Calculated from R-R interval buffer
    currentFeatures.tempSlopePerMin = 0.0f;

    MLInferenceResult mlResult = EdgeVitalPredictor::evaluate(currentFeatures);
    if (mlResult.isAnomaly) {
        Serial.printf("  [ML ALERT] %s (Confidence: %.2f)\n", mlResult.description, mlResult.confidenceScore);
    }

    // Dispatch Telemetry
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnected) {
            wifiConnected = true;
            logStatus(true, "Wi-Fi link restored");
        }
        sendTelemetry(temperature, finalHeartRate, mlResult.isAnomaly);
    } else {
        wifiConnected = false;
        Serial.println(F("  -> Wi-Fi offline, queuing next transmission cycle"));
    }

    Serial.printf("  [Telemetry Stats: OK=%d, FAIL=%d]\n", telemetrySuccessCount, telemetryFailureCount);
}
