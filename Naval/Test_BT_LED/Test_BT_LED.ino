/*
 * BT Debug: Check connection + receive
 * HC-05: TXD->Pin15, RXD->Pin14, STATE->Pin44
 */

#define BT_STATE 44

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(BT_STATE, INPUT);
  digitalWrite(13, LOW);
  Serial.println(">> BT Debug Ready");
}

void loop() {
  // Print connection status every 2 sec
  static unsigned long last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.println(">> Waiting for BT command...");
  }

  if (Serial2.available()) {
    char cmd = Serial2.read();
    Serial.print("BT got: [");
    Serial.print(cmd);
    Serial.print("] dec=");
    Serial.println((int)cmd);
    if (cmd == 'W' || cmd == 'w') { digitalWrite(13, HIGH); Serial.println(">> LED ON"); }
    if (cmd == 'U' || cmd == 'u') { digitalWrite(13, LOW); Serial.println(">> LED OFF"); }
  }

  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'W' || cmd == 'w') { digitalWrite(13, HIGH); Serial.println(">> LED ON"); }
    if (cmd == 'U' || cmd == 'u') { digitalWrite(13, LOW); Serial.println(">> LED OFF"); }
  }
}
