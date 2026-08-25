#ifndef CONFIG_H
#define CONFIG_H

// ==============================================================================
// Display Node Configuration Template
// CrowPanel 2.4" ESP32 Display & SoftAP Web Hub
// ==============================================================================

// --- Hardware Pin Definitions ---
#define PIN_SPEAKER          26
#define PIN_BACKLIGHT        27
#define PIN_BOOT_BUTTON      0
#define PIN_SHT31_SDA        21
#define PIN_SHT31_SCL        22

// --- Display Controller (ILI9341 SPI) ---
#define SPI_PIN_SCLK         14
#define SPI_PIN_MOSI         13
#define SPI_PIN_MISO         12
#define SPI_PIN_DC           2
#define SPI_PIN_CS           15
#define SPI_WRITE_FREQ       40000000

// --- Wireless Access Point Credentials ---
#define SOFTAP_SSID          "BabyMonitor"
#define SOFTAP_PASSWORD      "12345678"
#define HTTP_SERVER_PORT     80

// --- Default Thresholds & Calibration ---
#define DEFAULT_TEMP_TRIM    4.0f
#define THRESHOLD_TEMP_HIGH  38.0f
#define THRESHOLD_TEMP_LOW   35.0f
#define THRESHOLD_HR_HIGH    160
#define THRESHOLD_HR_LOW     50
#define THRESHOLD_ROOM_HIGH  28.0f
#define THRESHOLD_ROOM_LOW   18.0f

#endif // CONFIG_H
