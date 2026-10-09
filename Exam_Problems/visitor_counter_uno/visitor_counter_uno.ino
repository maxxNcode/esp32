/*
  Problem 10 - Smart Visitor Counter (Arduino Uno)

  - PIR motion sensor detects a visitor
  - Each VALID detection adds 1 to the counter:
      * only the START of a motion (LOW -> HIGH) counts
      * the PIR must then stay quiet (LOW) for REARM_MS before the next visitor can count,
        so one long / continuous motion is counted only once
  - 7-segment display shows the count (last digit when the count is over 9)
  - I2C LCD shows "Visitors: XX"
  - Buzzer: short beep for every valid detection
  - Push button: resets the counter to zero

  Board: Arduino Uno.  Library: LiquidCrystal I2C (Frank de Brabander). Wire is built in.

  No resistors: the 7-segment is driven ONE segment at a time with a short on-time
  (scan / duty cycle), so the current stays low. With 220 ohm resistors you can set
  SEG_ON_US to 2000 and SEG_OFF_US to 0 for a brighter display.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pins ----------
const int PIR_PIN    = 2;    // D2  <- PIR OUT
const int BUTTON_PIN = 3;    // D3  <- reset button (other leg to GND)
const int SEG_PINS[7] = {4, 5, 6, 7, 8, 9, 10};   // segments a, b, c, d, e, f, g
const int BUZZER_PIN = 12;   // D12 -> buzzer +
const int BOARD_LED  = 13;   // built-in L LED: ON while the PIR sees motion
// LCD: SDA = A4, SCL = A5

// ---------- Settings ----------
const bool COMMON_ANODE = false;          // true if your 7-segment COM goes to 5V
const unsigned long REARM_MS  = 1000;     // PIR must be LOW this long before the next count
const unsigned long WARMUP_MS = 15000;    // PIR needs time to settle after power-on
const unsigned int  SEG_ON_US  = 1000;    // each segment on for 1 ms ...
const unsigned int  SEG_OFF_US = 1000;    // ... then 1 ms dark (keeps current low)

LiquidCrystal_I2C lcd(0x27, 16, 2);       // if the LCD shows nothing, try 0x3F

// Which segments light for each digit. Bit 0 = a ... bit 6 = g
const byte DIGITS[10] = {
  0b0111111,  // 0
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0000111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};

unsigned int visitors = 0;
bool lastPir = false;
bool armed = false;               // true = ready to count the next visitor
unsigned long lowSince = 0;       // when the PIR last went LOW
bool warmedUp = false;
int lastWarmShown = -1;

String status = "";
unsigned long statusUntil = 0;

// Button (debounced, counts once per press)
int btnStable = HIGH, btnLast = HIGH;
unsigned long btnChangedAt = 0;

// 7-segment scan state
int scanSeg = 0;
bool scanOn = false;
unsigned long scanAt = 0;

// ---------- 7-segment (one segment at a time) ----------
void segWrite(int s, bool on) {
  if (COMMON_ANODE) on = !on;
  digitalWrite(SEG_PINS[s], on ? HIGH : LOW);
}

void refreshDisplay() {
  unsigned long nowUs = micros();
  if (scanOn) {
    if (nowUs - scanAt >= SEG_ON_US) {         // segment time over -> dark gap
      segWrite(scanSeg, false);
      scanOn = false;
      scanAt = nowUs;
    }
  } else if (nowUs - scanAt >= SEG_OFF_US) {   // gap over -> next segment
    scanSeg = (scanSeg + 1) % 7;
    if (DIGITS[visitors % 10] & (1 << scanSeg)) segWrite(scanSeg, true);
    scanOn = true;
    scanAt = nowUs;
  }
}

// ---------- LCD ----------
void showStatus(const String &msg, unsigned long ms) {   // LCD line 2
  status = msg;
  statusUntil = ms ? millis() + ms : 0;        // 0 = stays until replaced
  lcd.setCursor(0, 1);
  String line = msg;
  while (line.length() < 16) line += " ";
  lcd.print(line.substring(0, 16));
}

void showCount() {
  lcd.setCursor(0, 0);
  String line = "Visitors: ";
  if (visitors < 10) line += "0";              // "Visitors: 05"
  line += String(visitors);
  while (line.length() < 16) line += " ";
  lcd.print(line);
}

// ---------- Actions ----------
void countVisitor() {
  visitors++;
  showCount();
  showStatus("Visitor detected", 1500);
  tone(BUZZER_PIN, 2000, 150);                 // short beep
  Serial.print("Visitor! Count = ");
  Serial.println(visitors);
}

void resetCounter() {
  visitors = 0;
  showCount();
  showStatus("Counter reset", 1500);
  Serial.println("Counter reset to 0");
}

bool buttonPressed() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != btnLast) {
    btnLast = reading;
    btnChangedAt = millis();
  }
  if (millis() - btnChangedAt > 50 && reading != btnStable) {
    btnStable = reading;
    return reading == LOW;                     // pressed (INPUT_PULLUP)
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);
  for (int s = 0; s < 7; s++) {
    pinMode(SEG_PINS[s], OUTPUT);
    segWrite(s, false);
  }

  lcd.init();
  lcd.backlight();
  showCount();
  showStatus("PIR warming up", 0);
  Serial.println("Smart Visitor Counter ready (PIR warming up)");
}

void loop() {
  refreshDisplay();
  unsigned long now = millis();

  // 1) PIR warm-up: ignore the sensor for the first WARMUP_MS
  if (!warmedUp) {
    int left = (WARMUP_MS - now + 999) / 1000;
    if (now >= WARMUP_MS) {
      warmedUp = true;
      lowSince = now;
      lastPir = (digitalRead(PIR_PIN) == HIGH);
      showStatus("Ready", 0);
      Serial.println("PIR ready");
    } else if (left != lastWarmShown) {
      lastWarmShown = left;
      showStatus("PIR warm-up " + String(left) + "s", 0);
    }
  }

  // 2) Visitor detection
  if (warmedUp) {
    bool pir = (digitalRead(PIR_PIN) == HIGH);
    digitalWrite(BOARD_LED, pir ? HIGH : LOW);

    if (pir && !lastPir && armed) {      // NEW motion started -> one visitor
      countVisitor();
      armed = false;                     // ignore the rest of this motion
    }
    if (!pir) {
      if (lastPir) lowSince = now;       // motion just ended
      if (!armed && now - lowSince >= REARM_MS) armed = true;
    }
    lastPir = pir;
  }

  // 3) Reset button
  if (buttonPressed()) resetCounter();

  // 4) After a short message, go back to "Ready"
  if (statusUntil != 0 && now >= statusUntil) {
    showStatus("Ready", 0);
  }
}
