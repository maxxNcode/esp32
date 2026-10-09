/*
  Problem 16 - Smart Plant Monitoring System (Arduino Uno)

  - DHT11 humidity sensor: environmental humidity (read every 2 s)
  - LDR: available light (read every 200 ms), shown as 0-100 %
  - I2C LCD: humidity and light status, 3 display modes (button)
  - LED: ON when it is too dark   (light below DARK_PERCENT)
  - Buzzer: beeps when humidity falls below HUMIDITY_MIN (the threshold)
  - Push button: switches OVERVIEW -> HUMIDITY -> LIGHT -> OVERVIEW ...
  - Runs continuously

  LCD modes:
    OVERVIEW          HUMIDITY              LIGHT  (r = raw A0 value 0-1023)
    H:65% Light:80%   Humidity: 65.0%       Light:80% r205
    Status: OK        Min 40%: OK           LED OFF - BRIGHT

  Board: Arduino Uno.
  Libraries: DHT sensor library + Adafruit Unified Sensor (Adafruit),
             LiquidCrystal I2C (Frank de Brabander). Wire is built in.
  No resistors: bare LDR uses the Uno's internal pull-up on A0, the DHT library uses the
  internal pull-up on D2, the LED runs at a low PWM level.
*/

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pins ----------
const int DHT_PIN    = 2;    // D2  <- DHT11 DATA / OUT / S
const int BUTTON_PIN = 3;    // D3  <- mode button (other leg to GND)
const int LED_PIN    = 6;    // D6  -> LED long leg (PWM)
const int BUZZER_PIN = 12;   // D12 -> buzzer +
const int BOARD_LED  = 13;   // built-in L LED copies the LED
const int LDR_PIN    = A0;   // A0  <- LDR (other LDR leg to GND)
// LCD: SDA = A4, SCL = A5

// ---------- Settings ----------
#define DHT_TYPE DHT11                     // DHT22 if you have the white sensor
const float HUMIDITY_MIN = 40.0;           // humidity BELOW this -> buzzer (too dry)
const int   DARK_PERCENT = 30;             // light BELOW this % -> too dark -> LED ON
const int   HYSTERESIS   = 5;              // light must rise above 35 % to turn the LED off again
const bool  BARE_LDR     = true;           // true: bare LDR A0->GND (internal pull-up)
                                           // false: LDR MODULE (VCC, GND, AO -> A0)
const bool  INVERT_LIGHT = false;          // set true if light % goes DOWN when you add light
const unsigned long DHT_MS = 2000;         // DHT11 needs ~2 s between readings
const unsigned long LDR_MS = 200;
const int LED_LEVEL = 40;                  // 0-255, low because there is no LED resistor

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);        // if the LCD shows nothing, try 0x3F

float humidity = NAN;
bool haveHumidity = false;
int lightRaw = 0;          // 0-1023 from A0
int lightPercent = 0;      // 0 = dark, 100 = bright
bool tooDark = false;
bool tooDry = false;
int mode = 0;              // 0 = OVERVIEW, 1 = HUMIDITY, 2 = LIGHT

unsigned long lastDht = 0, lastLdr = 0, lastBeep = 0;
bool beepOn = false;
int btnStable = HIGH, btnLast = HIGH;
unsigned long btnChangedAt = 0;

// ---------- Helpers ----------
void lcdLine(int row, String text) {        // print exactly 16 characters
  while (text.length() < 16) text += " ";
  lcd.setCursor(0, row);
  lcd.print(text.substring(0, 16));
}

String humText() {                          // "65%" or "--%"
  return haveHumidity ? String((int)(humidity + 0.5)) + "%" : String("--%");
}

void updateLcd() {
  if (mode == 0) {                                          // OVERVIEW
    lcdLine(0, "H:" + humText() + " Light:" + String(lightPercent) + "%");
    String st;
    if (!haveHumidity)        st = "Status: NO DHT";
    else if (tooDry && tooDark) st = "Status: DRY+DARK";
    else if (tooDry)          st = "Status: DRY";
    else if (tooDark)         st = "Status: DARK";
    else                      st = "Status: OK";
    lcdLine(1, st);
  } else if (mode == 1) {                                   // HUMIDITY
    lcdLine(0, haveHumidity ? "Humidity: " + String(humidity, 1) + "%" : String("Humidity: --"));
    String line = "Min " + String((int)HUMIDITY_MIN) + "%: ";
    if (!haveHumidity) line += "NO DHT";
    else line += tooDry ? "TOO DRY" : "OK";
    lcdLine(1, line);
  } else {                                                  // LIGHT
    lcdLine(0, "Light:" + String(lightPercent) + "% r" + String(lightRaw));
    lcdLine(1, tooDark ? "LED ON  - DARK" : "LED OFF - BRIGHT");
  }
}

bool buttonPressed() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != btnLast) {
    btnLast = reading;
    btnChangedAt = millis();
  }
  if (millis() - btnChangedAt > 50 && reading != btnStable) {
    btnStable = reading;
    return reading == LOW;                   // pressed (INPUT_PULLUP)
  }
  return false;
}

void readLight() {
  lightRaw = analogRead(LDR_PIN);
  // Bare LDR with pull-up (and most LDR modules): MORE light -> LOWER reading
  int pct = map(lightRaw, 0, 1023, 100, 0);
  if (INVERT_LIGHT) pct = 100 - pct;
  lightPercent = constrain(pct, 0, 100);

  if (!tooDark && lightPercent < DARK_PERCENT) tooDark = true;                     // got dark
  else if (tooDark && lightPercent > DARK_PERCENT + HYSTERESIS) tooDark = false;   // bright again
}

void readHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("DHT read failed - check wiring");
    return;                                  // keep the last good value
  }
  humidity = h;
  haveHumidity = true;
  tooDry = humidity < HUMIDITY_MIN;
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, BARE_LDR ? INPUT_PULLUP : INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcdLine(0, "Plant Monitor");
  lcdLine(1, "Starting...");
  delay(1500);                               // let the DHT11 settle
  readHumidity();
  readLight();
  lastDht = lastLdr = millis();
  updateLcd();
}

void loop() {
  unsigned long now = millis();

  // 1) Continuous monitoring
  if (now - lastLdr >= LDR_MS) {
    lastLdr = now;
    readLight();
    updateLcd();
  }
  if (now - lastDht >= DHT_MS) {
    lastDht = now;
    readHumidity();
    updateLcd();
    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.print(" %  Light: ");
    Serial.print(lightPercent);
    Serial.print(" % (raw ");
    Serial.print(lightRaw);
    Serial.print(")  ");
    Serial.print(tooDry ? "DRY " : "");
    Serial.println(tooDark ? "DARK" : "");
  }

  // 2) LED ON when too dark
  analogWrite(LED_PIN, tooDark ? LED_LEVEL : 0);
  digitalWrite(BOARD_LED, tooDark ? HIGH : LOW);

  // 3) Buzzer beeps when humidity is below the threshold (300 ms on / 300 ms off)
  if (tooDry) {
    if (now - lastBeep >= 300) {
      lastBeep = now;
      beepOn = !beepOn;
      if (beepOn) tone(BUZZER_PIN, 2000);
      else noTone(BUZZER_PIN);
    }
  } else if (beepOn) {
    noTone(BUZZER_PIN);
    beepOn = false;
  }

  // 4) Button: next display mode
  if (buttonPressed()) {
    mode = (mode + 1) % 3;
    updateLcd();
  }
}
