/*
  Attendance Counter with Bluetooth Reporting (Arduino Uno)

  - Button 1 (D12): register attendance -> count + 1, beep
  - Button 2 (A0):  reset count to 0
  - 7-segment display (D2-D8): shows the count (last digit if count > 9)
  - I2C LCD 16x2 (A4/A5): shows the full count
  - HC-05 (D10/D11): phone sends "C" (or "?") -> Arduino replies with the count
  - Buzzer (D9): beeps on every valid registration

  Libraries: LiquidCrystal I2C (Frank de Brabander). SoftwareSerial and Wire are built in.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// ---------- Pins ----------
const int SEG_PINS[7] = {2, 3, 4, 5, 6, 7, 8};  // segments a, b, c, d, e, f, g
const int BUZZER    = 9;
const int BT_RX     = 10;   // connect to HC-05 TXD
const int BT_TX     = 11;   // connect to HC-05 RXD (through the 1k/2k divider)
const int BTN_COUNT = 12;   // attendance button -> GND
const int BTN_RESET = A0;   // reset button -> GND

// Set to true if your 7-segment is COMMON ANODE (COM pin goes to 5V)
const bool COMMON_ANODE = false;

SoftwareSerial bt(BT_RX, BT_TX);
LiquidCrystal_I2C lcd(0x27, 16, 2);   // if the LCD stays blank, try 0x3F

int count = 0;

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

// ---------- Button with debounce: true only once per press ----------
struct Button {
  int pin;
  int stable;
  int lastRead;
  unsigned long changedAt;
};

Button countBtn = {BTN_COUNT, HIGH, HIGH, 0};
Button resetBtn = {BTN_RESET, HIGH, HIGH, 0};

bool wasPressed(Button &b) {
  int reading = digitalRead(b.pin);
  if (reading != b.lastRead) {      // contact is bouncing / changing
    b.lastRead = reading;
    b.changedAt = millis();
  }
  if (millis() - b.changedAt > 50 && reading != b.stable) {
    b.stable = reading;             // steady for 50 ms -> real change
    return reading == LOW;          // LOW = pressed (INPUT_PULLUP)
  }
  return false;
}

// ---------- Outputs ----------
void showDigit(int d) {
  for (int s = 0; s < 7; s++) {
    bool on = DIGITS[d] & (1 << s);
    if (COMMON_ANODE) on = !on;
    digitalWrite(SEG_PINS[s], on ? HIGH : LOW);
  }
}

void showCount() {
  showDigit(count % 10);            // 7-segment shows 0-9 (last digit)

  lcd.setCursor(0, 0);
  lcd.print("Attendance:     ");
  lcd.setCursor(0, 1);
  lcd.print("Count: ");
  lcd.print(count);
  lcd.print("        ");            // clear leftover characters
}

void sendCount() {
  bt.print("Attendance count: ");
  bt.println(count);
  Serial.print("Sent to phone: ");
  Serial.println(count);
}

void handleRequest(char c) {
  if (c == 'C' || c == 'c' || c == '?') {
    sendCount();
  }
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(9600);
  bt.begin(9600);                   // HC-05 default baud rate

  for (int s = 0; s < 7; s++) pinMode(SEG_PINS[s], OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_COUNT, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  showCount();

  bt.println("Attendance counter ready. Send C to get the count.");
  Serial.println("Ready. Type C in Serial Monitor or send C from the phone.");
}

void loop() {
  // 1) Register attendance
  if (wasPressed(countBtn)) {
    count++;
    showCount();
    tone(BUZZER, 2000, 150);        // beep for every valid registration
    Serial.print("Attendance registered: ");
    Serial.println(count);
  }

  // 2) Reset
  if (wasPressed(resetBtn)) {
    count = 0;
    showCount();
    lcd.setCursor(0, 0);
    lcd.print("Count RESET     ");
    Serial.println("Count reset to 0");
  }

  // 3) Phone request through HC-05 (and Serial Monitor for testing)
  while (bt.available())     handleRequest(bt.read());
  while (Serial.available()) handleRequest(Serial.read());
}
