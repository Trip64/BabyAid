/**
 * @file display_node.ino
 * @brief CrowPanel 2.4" Display Node & Access Point Hub
 * @details Hosts local SoftAP and HTTP REST API for wearable sensor telemetry,
 *          samples local room ambient temperature & humidity via SHT31,
 *          renders real-time vital sign metrics on 320x240 ILI9341 TFT,
 *          and provides embedded responsive web dashboard.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "webapp.h"
#include "config.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// --- CrowPanel 2.4" ILI9341 Display Driver Configuration ---
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = SPI_WRITE_FREQ;
            cfg.pin_sclk = SPI_PIN_SCLK;
            cfg.pin_mosi = SPI_PIN_MOSI;
            cfg.pin_miso = SPI_PIN_MISO;
            cfg.pin_dc   = SPI_PIN_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = SPI_PIN_CS;
            cfg.pin_rst = -1;
            cfg.bus_shared = true;
            cfg.offset_rotation = 0;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

LGFX tft;
WebServer server(HTTP_SERVER_PORT);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// --- Display Color Palette (16-bit RGB565) ---
#define COLOR_BG          0x10A2
#define COLOR_PANEL       0x18E3
#define COLOR_HR          0x2F1A
#define COLOR_TEMP        0x6DDF
#define COLOR_ROOM_TEMP   0xFDA0
#define COLOR_ROOM_HUM    0xAD5F
#define COLOR_ALERT       0xF986
#define COLOR_WARN        0xFEA0
#define COLOR_NORMAL      0x4ECC
#define COLOR_WHITE       0xFFFF
#define COLOR_MUTED_WHITE 0xDEFB

// --- System State & Vital Sign Readings ---
float infantBodyTemp      = 0.0f;
long  infantHeartRate     = 0;
float ambientRoomTemp     = 0.0f;
float ambientRoomHumidity = 0.0f;

unsigned long lastTelemetryTimestamp = 0;
bool  isAlertActive       = false;
bool  isAudioMuted        = true;
float temperatureTrim     = DEFAULT_TEMP_TRIM;
String alertStatusMessage = "SYSTEM INITIALIZED";

// --- Visual Pulse & Buzzer Timing ---
int  displayHeartRate     = 0;
bool heartBeatPulseState  = false;
unsigned long lastBeatToggleMillis   = 0;
unsigned long lastBuzzerToggleMillis = 0;
bool buzzerState          = false;

// --- HTTP API Handlers ---
void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleApiState() {
    String json = "{";
    json += "\"bt\":" + String(infantBodyTemp, 1) + ",";
    json += "\"hr\":" + String(displayHeartRate) + ",";
    json += "\"rt\":" + String(ambientRoomTemp, 1) + ",";
    json += "\"rh\":" + String(ambientRoomHumidity, 0) + ",";
    json += "\"alert\":" + String(isAlertActive ? "true" : "false") + ",";
    json += "\"msg\":\"" + alertStatusMessage + "\",";
    json += "\"mute\":" + String(isAudioMuted ? "true" : "false") + ",";
    json += "\"trim\":" + String(temperatureTrim, 1);
    json += "}";
    server.send(200, "application/json", json);
}

void handleApiSettings() {
    if (server.hasArg("trim")) {
        temperatureTrim = server.arg("trim").toFloat();
    }
    if (server.hasArg("mute")) {
        isAudioMuted = (server.arg("mute").toInt() == 1);
    }
    server.send(200, "text/plain", "OK");
}

void handleTelemetryPost() {
    if (server.hasArg("temp")) {
        infantBodyTemp = server.arg("temp").toFloat() + temperatureTrim;
    }
    if (server.hasArg("ir")) {
        infantHeartRate = server.arg("ir").toInt();
    }
    lastTelemetryTimestamp = millis();
    server.send(200, "text/plain", "OK");
    evaluateAlertThresholds();
}

/**
 * @brief Evaluates physiological and environmental safety bounds.
 */
void evaluateAlertThresholds() {
    isAlertActive = false;
    alertStatusMessage = "ALL PARAMETERS NORMAL";

    if (infantBodyTemp > THRESHOLD_TEMP_HIGH) {
        isAlertActive = true;
        alertStatusMessage = "! HIGH INFANT TEMPERATURE !";
    } else if (displayHeartRate > THRESHOLD_HR_HIGH) {
        isAlertActive = true;
        alertStatusMessage = "! ELEVATED HEART RATE !";
    } else if (displayHeartRate < THRESHOLD_HR_LOW && displayHeartRate > 0) {
        isAlertActive = true;
        alertStatusMessage = "! LOW HEART RATE (BRADYCARDIA) !";
    } else if (ambientRoomTemp > THRESHOLD_ROOM_HIGH) {
        isAlertActive = true;
        alertStatusMessage = "! ROOM TEMPERATURE TOO HIGH !";
    } else if (ambientRoomTemp < THRESHOLD_ROOM_LOW && ambientRoomTemp > 0) {
        isAlertActive = true;
        alertStatusMessage = "! ROOM TEMPERATURE TOO LOW !";
    }
}

/**
 * @brief Draws a stylized dashboard card container.
 */
void drawCard(int x, int y, int w, int h, uint32_t borderColor) {
    tft.fillRoundRect(x, y, w, h, 12, COLOR_PANEL);
    tft.drawRoundRect(x, y, w, h, 12, borderColor);
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 11, borderColor);
}

/**
 * @brief Renders the primary dashboard UI to the TFT display.
 */
void renderDashboard() {
    tft.startWrite();
    tft.fillScreen(COLOR_BG);

    // Header Bar
    uint32_t headerBg = isAlertActive ? COLOR_ALERT : COLOR_PANEL;
    tft.fillRoundRect(4, 2, 312, 24, 8, headerBg);
    tft.setTextColor(isAlertActive ? COLOR_WHITE : COLOR_MUTED_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 9);
    tft.print(F("KIDSENTINEL MONITOR"));

    bool telemetryLive = (millis() - lastTelemetryTimestamp) < 5000;
    tft.setCursor(170, 9);
    tft.setTextColor(telemetryLive ? COLOR_NORMAL : COLOR_ALERT);
    tft.fillSmoothCircle(164, 13, 3, telemetryLive ? COLOR_NORMAL : COLOR_ALERT);
    tft.print(telemetryLive ? F("LINK OK") : F("NO LINK"));

    tft.setCursor(240, 9);
    tft.setTextColor(isAudioMuted ? COLOR_WARN : COLOR_NORMAL);
    tft.print(isAudioMuted ? F("[MUTED]") : F("[AUDIO]"));

    // Card 1: Infant Body Temperature
    drawCard(4, 30, 152, 80, COLOR_TEMP);
    tft.setTextColor(COLOR_TEMP);
    tft.setTextSize(1);
    tft.setCursor(16, 38);
    tft.print(F("INFANT TEMP"));
    tft.setTextSize(4);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(16, 56);
    if (infantBodyTemp > 0.0f) {
        tft.printf("%.1f", infantBodyTemp);
        tft.setTextSize(2);
        tft.print(F("C"));
    } else {
        tft.print(F("--.-"));
    }

    // Card 2: Infant Heart Rate & Optical Beat Indicator
    drawCard(164, 30, 152, 80, COLOR_HR);
    tft.setTextColor(COLOR_HR);
    tft.setTextSize(1);
    tft.setCursor(176, 38);
    tft.print(F("HEART RATE"));
    tft.setCursor(276, 38);
    tft.print(F("BPM"));
    tft.fillSmoothCircle(302, 40, heartBeatPulseState ? 6 : 4, heartBeatPulseState ? 0xF800 : 0x8000);
    tft.setTextSize(4);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(176, 56);
    if (displayHeartRate > 0) {
        tft.printf("%3d", displayHeartRate);
    } else {
        tft.print(F("---"));
    }

    // Card 3: Room Ambient Temperature
    drawCard(4, 116, 152, 80, COLOR_ROOM_TEMP);
    tft.setTextColor(COLOR_ROOM_TEMP);
    tft.setTextSize(1);
    tft.setCursor(16, 124);
    tft.print(F("ROOM TEMP"));
    tft.setTextSize(4);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(16, 142);
    if (ambientRoomTemp > 0.0f) {
        tft.printf("%.1f", ambientRoomTemp);
        tft.setTextSize(2);
        tft.print(F("C"));
    } else {
        tft.print(F("--.-"));
    }

    // Card 4: Room Ambient Humidity
    drawCard(164, 116, 152, 80, COLOR_ROOM_HUM);
    tft.setTextColor(COLOR_ROOM_HUM);
    tft.setTextSize(1);
    tft.setCursor(176, 124);
    tft.print(F("ROOM HUMIDITY"));
    tft.setTextSize(4);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(176, 142);
    if (ambientRoomHumidity > 0.0f) {
        tft.printf("%.0f", ambientRoomHumidity);
        tft.setTextSize(2);
        tft.print(F("%"));
    } else {
        tft.print(F("--"));
    }

    // Footer Alert Banner
    uint32_t footerBg = isAlertActive ? 0x6000 : COLOR_PANEL;
    tft.fillRoundRect(4, 202, 312, 32, 8, footerBg);
    tft.drawRoundRect(4, 202, 312, 32, 8, isAlertActive ? COLOR_ALERT : 0x4A49);
    tft.setTextColor(isAlertActive ? COLOR_WHITE : COLOR_MUTED_WHITE);
    tft.setTextSize(1);
    tft.setCursor(14, 215);
    tft.print(alertStatusMessage);

    tft.endWrite();
}

void setup() {
    Serial.begin(115200);
    
    // Configure IO
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);
    pinMode(PIN_SPEAKER, OUTPUT);
    digitalWrite(PIN_SPEAKER, LOW);
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

    // Initialize Graphic Display
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(COLOR_BG);

    // Initialize Environmental Sensor
    Wire.begin(PIN_SHT31_SDA, PIN_SHT31_SCL);
    if (sht31.begin(0x44) || sht31.begin(0x45)) {
        Serial.println(F("[SHT31] Environmental sensor initialized"));
    } else {
        Serial.println(F("[SHT31] Sensor initialization failed"));
    }

    // Initialize SoftAP & Web Server
    WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSWORD);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    Serial.printf("[WiFi] SoftAP started. IP: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/state", HTTP_GET, handleApiState);
    server.on("/api/settings", HTTP_POST, handleApiSettings);
    server.on("/data", HTTP_POST, handleTelemetryPost);

    server.begin();
    Serial.println(F("[HTTP] Web server listening on port 80"));
}

void loop() {
    server.handleClient();

    // Poll SHT31 Environmental Sensor
    static unsigned long lastSensorPoll = 0;
    if (millis() - lastSensorPoll > 2000) {
        float temp = sht31.readTemperature();
        float hum  = sht31.readHumidity();
        if (!isnan(temp)) ambientRoomTemp = temp;
        if (!isnan(hum))  ambientRoomHumidity = hum;
        lastSensorPoll = millis();
        evaluateAlertThresholds();
    }

    // Hardware Button Mute Toggle (GPIO 0)
    static bool previousButtonState = HIGH;
    bool currentButtonState = digitalRead(PIN_BOOT_BUTTON);
    if (currentButtonState == LOW && previousButtonState == HIGH) {
        isAudioMuted = !isAudioMuted;
        tone(PIN_SPEAKER, 500, 50);
        delay(150);
    }
    previousButtonState = currentButtonState;

    displayHeartRate = (infantHeartRate > 0) ? infantHeartRate : 0;

    // Optical Heart Beat Indicator Animation
    unsigned long beatInterval = (displayHeartRate > 0) ? (60000UL / displayHeartRate) : 700;
    if (millis() - lastBeatToggleMillis > (beatInterval / 2)) {
        heartBeatPulseState = !heartBeatPulseState;
        lastBeatToggleMillis = millis();
    }

    // Non-blocking Audible Alarm
    if (isAlertActive && !isAudioMuted) {
        if (millis() - lastBuzzerToggleMillis > 500) {
            buzzerState = !buzzerState;
            if (buzzerState) tone(PIN_SPEAKER, 1000, 150);
            lastBuzzerToggleMillis = millis();
        }
    } else {
        buzzerState = false;
    }

    // Refresh TFT UI
    static unsigned long lastDisplayRefresh = 0;
    if (millis() - lastDisplayRefresh > 500) {
        renderDashboard();
        lastDisplayRefresh = millis();
    }
}
