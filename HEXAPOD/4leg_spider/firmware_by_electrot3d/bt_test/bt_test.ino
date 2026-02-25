#include <SoftwareSerial.h>

SoftwareSerial BT(3, 2);
const int ledPin = 13;

void setup() {
  Serial.begin(9600);
  BT.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Start OFF
  Serial.println("Ready. W=ON, F=OFF");
}

void loop() {
  if (BT.available()) {
    char cmd = BT.read();
    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == 'W') {        // W = LED ON
      digitalWrite(ledPin, HIGH);
      Serial.println("LED ON");
    }
    else if (cmd == 'F') {   // F = LED OFF
      digitalWrite(ledPin, LOW);
      Serial.println("LED OFF");
    }
  }
}
