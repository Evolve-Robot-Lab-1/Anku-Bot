/*
 * I2C Scanner - Check if PCA9685 is detected
 */

#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial);

  Serial.println("\n===== I2C SCANNER =====");
  Serial.println("Scanning for devices...\n");
}

void loop() {
  byte count = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("FOUND device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);

      if (addr == 0x40) {
        Serial.print(" <-- PCA9685 (default)");
      } else if (addr == 0x70) {
        Serial.print(" <-- PCA9685 (all-call)");
      }
      Serial.println();
      count++;
    }
  }

  Serial.println();
  if (count == 0) {
    Serial.println("ERROR: No I2C devices found!");
    Serial.println("Check wiring:");
    Serial.println("  - SDA -> A4");
    Serial.println("  - SCL -> A5");
    Serial.println("  - VCC -> 5V");
    Serial.println("  - GND -> GND");
  } else {
    Serial.print("Found ");
    Serial.print(count);
    Serial.println(" device(s)");

    if (count > 0) {
      Serial.println("\nPCA9685 should be at 0x40");
    }
  }

  Serial.println("\nScanning again in 5 seconds...\n");
  delay(5000);
}
