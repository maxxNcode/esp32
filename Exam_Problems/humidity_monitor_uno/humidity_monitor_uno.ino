/*
  Problem 4 - Smart Temperature & Humidity Monitor (Arduino Uno)

  - DHT11 humidity sensor: humidity (and temperature) read every 2 seconds (continuous update)
  - I2C LCD: shows humidity and system status
  - Warning LED: ON when humidity > 70 %
  - Buzzer: beeps when humidity > 80 %
  - Push button: switches between NORMAL and DETAILED display mode

      Status      Humidity        LED   Buzzer
      NORMAL      70 % or less    off   off
      WARNING     above 70 %      ON    off
      ALARM       above 80 %      ON    beeping

  LCD, NORMAL mode:          LCD, DETAILED mode:
    Humidity: 65%              H:65.0% T:28.4C
    Status: NORMAL             NORMAL   all OFF

  Board: Arduino Uno.
  Libraries: DHT sensor library + Adafruit Unified Sensor (Adafruit),
             LiquidCrystal I2C (Frank de Brabander). Wire is built in.
  No resistors: the DHT library uses the Uno's internal pull-up, and the LED is driven
  with a low PWM level (LED_LEVEL).
*/

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pins ----------
const int DHT_PIN    = 2;    // D2  <- DHT11 DATA / OUT / S
const int BUTTON_PIN = 3;    // D3  <- mode button (other leg to GND)
const int LED_PIN    = 6;    // D6  -> warning LED long leg (PWM)
const int BUZZER_PIN = 12;   // D12 -> buzzer +
const int BOARD_LED  = 13;   // built-in L LED copies the warning LED
// LCD: SDA = A4, SCL = A5

// ---------- Settings ----------
#define DHT_TYPE DHT11                     // change to DHT22 if you have the white sensor
const float LED_LIMIT    = 70.0;           // humidity above this -> LED ON
const float BUZZER_LIMIT = 80.0;           // humidity above this -> buzzer
const unsigned long READ_MS = 2000;        // DHT11 needs about 1-2 s between readings
const int LED_LEVEL = 40;                  // 0-255, low because there is no LED resistor

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);        // if the LCD shows nothing, try 0x3F

enum Status { NORMAL, WARNING, ALARM, SENSOR_ERROR };
Status status = SENSOR_ERROR;
bool detailedMode = false;
float humidity = NAN, temperature = NAN;
bool haveReading = false;

unsigned long lastRead = 0, lastBeep = 0;
bool beepOn = false;

// Button (debounced, true once per press)
int btnStable = HIGH, btnLast = HIGH;
unsigned long btnChangedAt = 0;

// ---------- Helpers ----------
const char *statusText() {
  switch (status) {
    case NORMAL:  return "NORMAL";
    case WARNING: return "WARNING";
    case ALARM:   return "ALARM";
    default:      return "SENSOR ERR";
  }
}

void lcdLine(int row, String text) {        // print exactly 16 characters
  while (text.length() < 16) text += " ";
  lcd.setCursor(0, row);
  lcd.print(text.substring(0, 16));
}

void updateLcd() {
  if (!haveReading) {
    lcdLine(0, "Humidity: --%");
    lcdLine(1, status == SENSOR_ERROR && millis() > 5000 ? "Check sensor!" : "Reading...");
    return;
  }
  if (!detailedMode) {
    lcdLine(0, "Humidity: " + String((int)(humidity + 0.5)) + "%");
    lcdLine(1, String("Status: ") + statusText());
  } else {
    lcdLine(0, "H:" + String(humidity, 1) + "% T:" + String(temperature, 1) + "C");
    switch (status) {
      case NORMAL:  lcdLine(1, "NORMAL   all OFF"); break;
      case WARNING: lcdLine(1, "WARNING  LED ON"); break;
      case ALARM:   lcdLine(1, "ALARM LED+BUZZER"); break;
      default:      lcdLine(1, "SENSOR ERROR"); break;
    }
  }
}

void setLed(bool on) {
  analogWrite(LED_PIN, on ? LED_LEVEL : 0);
  digitalWrite(BOARD_LED, on ? HIGH : LOW);
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

void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();           // Celsius
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT read failed - check wiring");
    if (!haveReading) status = SENSOR_ERROR; // keep the last good value otherwise
    return;
  }
  humidity = h;
  temperature = t;
  haveReading = true;

  if (humidity > BUZZER_LIMIT)   status = ALARM;
  else if (humidity > LED_LIMIT) status = WARNING;
  else                           status = NORMAL;

  Serial.print("Humidity: ");
  Serial.print(humidity, 1);
  Serial.print(" %  Temp: ");
  Serial.print(temperature, 1);
  Serial.print(" C  Status: ");
  Serial.println(statusText());
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setLed(false);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcdLine(0, "Humidity Monitor");
  lcdLine(1, "Starting...");
  delay(1500);                               // let the DHT11 settle
  readSensor();
  lastRead = millis();
  updateLcd();
}

void loop() {
  unsigned long now = millis();

  // 1) Read the sensor continuously (every READ_MS)
  if (now - lastRead >= READ_MS) {
    lastRead = now;
    readSensor();
    updateLcd();
  }

  // 2) Warning LED: ON above 70 %
  setLed(status == WARNING || status == ALARM);

  // 3) Buzzer: beeps above 80 % (200 ms on / 200 ms off)
  if (status == ALARM) {
    if (now - lastBeep >= 200) {
      lastBeep = now;
      beepOn = !beepOn;
      if (beepOn) tone(BUZZER_PIN, 2500);
      else noTone(BUZZER_PIN);
    }
  } else if (beepOn) {
    noTone(BUZZER_PIN);
    beepOn = false;
  }

  // 4) Button: switch NORMAL <-> DETAILED mode
  if (buttonPressed()) {
    detailedMode = !detailedMode;
    Serial.println(detailedMode ? "Mode: DETAILED" : "Mode: NORMAL");
    updateLcd();
  }
}
