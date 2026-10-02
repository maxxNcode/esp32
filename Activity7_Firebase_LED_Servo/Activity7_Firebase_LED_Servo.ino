/*
  Activity 7 - ESP32 + Firebase Realtime Database + Bluetooth + MIT App Inventor

  The MIT App Inventor app controls 4 LEDs and 1 servo in two ways:
    1. Bluetooth (when connected): sends lines like "LED1:1", "ALL_LED:0", "SERVO:90"
    2. Firebase (IoT):             writes LED1..LED4, ALL_LED, SERVO in the database

  Values:
      LED1, LED2, LED3, LED4  -> 0 (off) or 1 (on)
      ALL_LED                 -> 0 (all off) or 1 (all on)
      SERVO                   -> 0, 90 or 180 (angle in degrees)

  Bluetooth commands are also written to Firebase, so the database always
  matches the real LEDs/servo.

  Libraries (Arduino IDE > Tools > Manage Libraries):
    - "Firebase Arduino Client Library for ESP8266 and ESP32" by Mobizt
    - "ESP32Servo" by Kevin Harrington
  Board: ESP32 Dev Module (classic ESP32 - Bluetooth Serial does not work on ESP32-S3/C3)
  IMPORTANT: Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)"
             (Wi-Fi + Bluetooth + Firebase is too big for the default partition)
*/

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Firebase_ESP_Client.h>
#include <ESP32Servo.h>

// ---------- CHANGE THESE ----------
#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Realtime Database URL, without "https://" and without the trailing "/"
#define DATABASE_URL    "activity7-8fac0-default-rtdb.asia-southeast1.firebasedatabase.app"

// Project settings > Service accounts > Database secrets
#define DATABASE_SECRET "YOUR_DATABASE_SECRET"

// Name shown when the phone scans for Bluetooth devices
#define BT_NAME         "ESP32_Activity7"
// -----------------------------------

// Pins
const int LED_PINS[4] = {16, 17, 18, 19};  // LED1..LED4 (each through a 220-330 ohm resistor)
const int SERVO_PIN   = 13;                // servo signal (orange/yellow wire)

BluetoothSerial SerialBT;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo servo;

int lastAllLed   = -1;  // -1 = not read yet
int lastServo    = -1;
int ledState[4]  = {-1, -1, -1, -1};
String btLine;
unsigned long lastBtByte = 0;

unsigned long lastPoll = 0;
const unsigned long POLL_MS = 300;

bool firebaseOnline() {
  return WiFi.status() == WL_CONNECTED && Firebase.ready();
}

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

void setServo(int angle) {
  angle = constrain(angle, 0, 180);
  servo.write(angle);
  lastServo = angle;
  Serial.printf("SERVO -> %d deg\n", angle);
}

// ---------- Bluetooth ----------

// Apply one command from Bluetooth, e.g. key="LED1" value=1, then copy it to Firebase.
void applyBluetoothCommand(String key, int value) {
  FirebaseJson update;

  if (key == "ALL_LED") {
    int all = value ? 1 : 0;
    lastAllLed = all;
    for (int i = 0; i < 4; i++) {
      setLed(i, all);
      update.set("LED" + String(i + 1), all);
    }
    update.set("ALL_LED", all);
  } else if (key == "SERVO") {
    setServo(value);
    update.set("SERVO", lastServo);
  } else if (key.startsWith("LED") && key.length() == 4) {
    int i = key.substring(3).toInt() - 1;
    if (i < 0 || i > 3) return;
    setLed(i, value);
    update.set(key, ledState[i]);
  } else {
    Serial.println("Unknown Bluetooth command: " + key);
    return;
  }

  SerialBT.println("OK " + key + ":" + String(value));
  // Save to Firebase too, otherwise the next Firebase read would undo it
  if (firebaseOnline() && !Firebase.RTDB.updateNode(&fbdo, "/", &update)) {
    Serial.print("Firebase write failed: ");
    Serial.println(fbdo.errorReason());
  }
}

void processBtLine() {
  btLine.replace("\\n", "");  // in case "\n" arrived as two characters
  btLine.trim();
  if (btLine.length() == 0) return;
  Serial.println("BT received: " + btLine);
  int sep = btLine.indexOf(':');
  if (sep > 0) {
    applyBluetoothCommand(btLine.substring(0, sep), btLine.substring(sep + 1).toInt());
  } else {
    Serial.println("Bad Bluetooth command (expected KEY:VALUE)");
  }
  btLine = "";
}

// Read Bluetooth characters. A command ends with a newline, ';', or a
// 100 ms pause, so it works even if the app doesn't send a newline.
void handleBluetooth() {
  while (SerialBT.available()) {
    char c = SerialBT.read();
    lastBtByte = millis();
    if (c == '\n' || c == '\r' || c == ';') {
      processBtLine();
    } else if (btLine.length() < 32) {
      btLine += c;
    }
  }
  if (btLine.length() > 0 && millis() - lastBtByte > 100) processBtLine();
}

// Prints when a phone connects/disconnects over Bluetooth.
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.printf("Bluetooth: phone CONNECTED (free heap %u bytes)\n", ESP.getFreeHeap());
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Bluetooth: phone disconnected");
  }
}

// ---------- Firebase ----------

void pollFirebase() {
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
    if (angle != lastServo) setServo(angle);
  }
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

  SerialBT.register_callback(btCallback);
  SerialBT.begin(BT_NAME);
  Serial.println("Bluetooth ready: " BT_NAME);

  // Wait up to 15 s for Wi-Fi. Bluetooth still works without it.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo WiFi - Bluetooth only (will keep retrying WiFi)");
  }

  // Smaller SSL buffers leave enough memory for Bluetooth + Wi-Fi together
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  handleBluetooth();

  if (millis() - lastPoll < POLL_MS) return;
  lastPoll = millis();

  if (firebaseOnline()) pollFirebase();
}
