/**
 * @file i2c_scanner.ino
 * @brief Universal I2C Bus Hardware Diagnostic Utility
 * @details Scans active I2C addresses on ESP32 / ESP32-C6 to verify
 *          wiring of MAX30100 (0x57) and SHT31 (0x44/0x45).
 */

#include <Wire.h>

#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("       I2C BUS HARDWARE SCANNER         "));
    Serial.println(F("========================================"));

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
    byte error, address;
    int devicesFound = 0;

    Serial.println(F("[*] Scanning I2C bus..."));

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("  -> Detected device at address 0x%02X", address);
            if (address == 0x57) Serial.print(F(" (MAX30100 / MAX30102 PPG)"));
            else if (address == 0x44 || address == 0x45) Serial.print(F(" (SHT31 Environmental)"));
            Serial.println();
            devicesFound++;
        }
    }

    if (devicesFound == 0) {
        Serial.println(F("[!] No I2C devices detected. Check wiring and 3.3V power."));
    } else {
        Serial.printf("[✓] Scan complete. %d device(s) active on bus.\n", devicesFound);
    }

    Serial.println();
    delay(5000);
}
