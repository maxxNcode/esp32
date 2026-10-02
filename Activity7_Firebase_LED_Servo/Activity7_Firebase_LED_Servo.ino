/*
  Activity 7 - ESP32 + Firebase Realtime Database + MIT App Inventor

  The MIT App Inventor app writes values to Firebase:
      LED1, LED2, LED3, LED4  -> 0 (off) or 1 (on)
      ALL_LED                 -> 0 (all off) or 1 (all on)
      SERVO                   -> 0, 90 or 180 (angle in degrees)

  The ESP32 reads those values and drives 4 LEDs and 1 servo.

  Libraries (Arduino IDE > Tools > Manage Libraries):
    - "Firebase Arduino Client Library for ESP8266 and ESP32" by Mobizt
    - "ESP32Servo" by Kevin Harrington
  Board: ESP32 Dev Module (install "esp32 by Espressif Systems" in Boards Manager)
*/

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ESP32Servo.h>

// ---------- CHANGE THESE ----------
#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Realtime Database URL, without "https://" and without the trailing "/"
// e.g. "activity7-1234-default-rtdb.firebaseio.com"
#define DATABASE_URL    "YOUR_PROJECT-default-rtdb.firebaseio.com"

// Project settings > Service accounts > Database secrets
// (use the same secret as the FirebaseToken in MIT App Inventor)
#define DATABASE_SECRET "YOUR_DATABASE_SECRET"
// -----------------------------------

// Pins
const int LED_PINS[4] = {16, 17, 18, 19};  // LED1..LED4 (each through a 220-330 ohm resistor)
const int SERVO_PIN   = 13;                // servo signal (orange/yellow wire)

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo servo;

int lastAllLed   = -1;  // -1 = not read yet
int lastServo    = -1;
int ledState[4]  = {-1, -1, -1, -1};

unsigned long lastPoll = 0;
const unsigned long POLL_MS = 300;

// App Inventor may save numbers as 1 or as text "1" / "\"1\"".
// This turns any of those into an int.
int toInt(FirebaseJsonData &d) {
  String s = d.to<String>();
  s.replace("\"", "");
  s.trim();
  return s.toInt();
}

void setLed(int i, int value) {
  ledState[i] = value ? 1 : 0;
  digitalWrite(LED_PINS[i], ledState[i] ? HIGH : LOW);
  Serial.printf("LED%d -> %s\n", i + 1, ledState[i] ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);
  servo.write(0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.print("\nConnected, IP: ");
  Serial.println(WiFi.localIP());

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  if (millis() - lastPoll < POLL_MS) return;
  lastPoll = millis();

  // Read the whole database root in one request
  if (!Firebase.RTDB.getJSON(&fbdo, "/")) {
    Serial.print("Firebase read failed: ");
    Serial.println(fbdo.errorReason());
    return;
  }

  FirebaseJson &json = fbdo.jsonObject();
  FirebaseJsonData d;

  // ALL_LED: act only when its value CHANGES, so the single LED buttons
  // still work afterwards.
  if (json.get(d, "ALL_LED")) {
    int all = toInt(d) ? 1 : 0;
    if (all != lastAllLed) {
      bool firstRead = (lastAllLed == -1);
      lastAllLed = all;
      if (!firstRead) {  // don't override the LEDs right after boot
        for (int i = 0; i < 4; i++) setLed(i, all);

        // Copy the value into LED1..LED4 in Firebase so the database
        // matches the real LEDs, then wait for the next read.
        FirebaseJson update;
        for (int i = 0; i < 4; i++) update.set("LED" + String(i + 1), all);
        Firebase.RTDB.updateNode(&fbdo, "/", &update);
        return;
      }
    }
  }

  // LED1..LED4
  for (int i = 0; i < 4; i++) {
    String key = "LED" + String(i + 1);
    if (json.get(d, key)) {
      int v = toInt(d) ? 1 : 0;
      if (v != ledState[i]) setLed(i, v);
    }
  }

  // SERVO angle
  if (json.get(d, "SERVO")) {
    int angle = constrain(toInt(d), 0, 180);
    if (angle != lastServo) {
      servo.write(angle);
      lastServo = angle;
      Serial.printf("SERVO -> %d deg\n", angle);
    }
  }
}
