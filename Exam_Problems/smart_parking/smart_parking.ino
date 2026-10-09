/*
  Problem 19 - Smart Parking Monitoring with Mobile App (ESP32)

  - Ultrasonic sensor: object closer than OCCUPIED_CM -> slot OCCUPIED, else AVAILABLE
  - LED: ON = AVAILABLE, OFF = OCCUPIED
  - Servo: parking barrier (0 = closed, 90 = open)
  - I2C LCD: shows slot status, gate status and distance
  - MIT App Inventor app (Bluetooth): shows the status, OPEN GATE button sends "OPEN"
      * slot AVAILABLE -> gate opens for 5 s, then closes
      * slot OCCUPIED  -> gate stays closed, app shows "DENIED - SLOT OCCUPIED"

  Messages sent to the app (one per line):
      AVAILABLE / OCCUPIED            (every second and on every change)
      GATE OPENED / GATE CLOSED / DENIED - SLOT OCCUPIED

  Board: ESP32 Dev Module (classic ESP32 with Bluetooth Classic)
  Libraries: ESP32Servo, LiquidCrystal I2C (Frank de Brabander)
*/

#include <BluetoothSerial.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "driver/gpio.h"     // for the LED current limit (no resistor needed)

// ---------- Pins ----------
// Numbers = GPIO numbers = the "G" labels on the board (G5, G18, G4, G13)
const int TRIG_PIN  = 5;    // G5  -> HC-SR04 Trig
const int ECHO_PIN  = 18;   // G18 -> HC-SR04 Echo (direct)
const int LED_PIN   = 4;    // G4  -> availability LED long leg (no resistor)
const int SERVO_PIN = 13;   // G13 -> barrier servo signal (orange)
// LCD: SDA = G21, SCL = G22 (ESP32 default I2C pins). Power from the 5V pin.

// ---------- Settings ----------
const int OCCUPIED_CM = 10;             // closer than this = car in the slot
const int GATE_CLOSED = 0;              // servo angle when closed
const int GATE_OPEN   = 90;             // servo angle when open
const unsigned long OPEN_TIME_MS = 5000;

BluetoothSerial bt;
Servo gate;
LiquidCrystal_I2C lcd(0x27, 16, 2);     // if the LCD stays blank, try 0x3F

bool occupied = false;
bool gateIsOpen = false;
unsigned long gateOpenedAt = 0;
unsigned long lastRead = 0, lastReport = 0;
long distanceCm = 999;
String cmd;
unsigned long lastCmdByte = 0;

// ---------- Ultrasonic ----------
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);   // 30 ms timeout
  if (duration == 0) return 999;                    // no echo = nothing in front
  return duration * 0.034 / 2;
}

// ---------- Outputs ----------
void send(const String &msg) {          // to the phone and the Serial Monitor
  bt.println(msg);
  Serial.println(msg);
}

void updateLcd() {
  lcd.setCursor(0, 0);
  lcd.print(occupied ? "Slot: OCCUPIED  " : "Slot: AVAILABLE ");
  lcd.setCursor(0, 1);
  lcd.print(gateIsOpen ? "Gate:OPEN " : "Gate:CLOSE");   // 10 chars + 6 chars distance = 16
  if (distanceCm >= 999) lcd.print("  --cm");
  else {
    String d = String(distanceCm) + "cm";
    while (d.length() < 6) d = " " + d;
    lcd.print(d);
  }
}

void openGate() {
  gate.write(GATE_OPEN);
  gateIsOpen = true;
  gateOpenedAt = millis();
  send("GATE OPENED");
}

void closeGate() {
  gate.write(GATE_CLOSED);
  gateIsOpen = false;
  send("GATE CLOSED");
}

// ---------- Commands from the app ----------
void handleCommand(String c) {
  c.trim();
  c.toUpperCase();
  if (c.length() == 0) return;
  Serial.println("Command: " + c);

  if (c == "OPEN" || c == "OPEN GATE") {
    if (occupied) {
      send("DENIED - SLOT OCCUPIED");     // do not open when the slot is occupied
    } else {
      openGate();
    }
  } else if (c == "CLOSE") {
    closeGate();
  } else if (c == "STATUS") {
    send(occupied ? "OCCUPIED" : "AVAILABLE");
  }
  updateLcd();
}

// A command ends with a newline, ';' or a 100 ms pause.
void readCommands(Stream &in) {
  while (in.available()) {
    char ch = in.read();
    lastCmdByte = millis();
    if (ch == '\n' || ch == '\r' || ch == ';') {
      handleCommand(cmd);
      cmd = "";
    } else if (cmd.length() < 20) {
      cmd += ch;
    }
  }
}

void setup() {
  Serial.begin(115200);
  bt.begin("ESP32_Parking");            // name shown when the phone scans

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  // No resistor on the LED: use the weakest pin drive (~5 mA) to protect LED and ESP32
  gpio_set_drive_capability((gpio_num_t)LED_PIN, GPIO_DRIVE_CAP_0);

  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(GATE_CLOSED);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

  Serial.println("Ready. Bluetooth name: ESP32_Parking. Type OPEN to test.");
}

void loop() {
  // 1) Read the sensor every 200 ms
  if (millis() - lastRead >= 200) {
    lastRead = millis();
    distanceCm = readDistanceCm();
    bool nowOccupied = (distanceCm < OCCUPIED_CM);

    if (nowOccupied != occupied) {      // status changed -> tell the app at once
      occupied = nowOccupied;
      send(occupied ? "OCCUPIED" : "AVAILABLE");
    }
    digitalWrite(LED_PIN, occupied ? LOW : HIGH);   // LED ON = available
    updateLcd();
  }

  // 2) Report the status every second (so the app is always up to date)
  if (millis() - lastReport >= 1000) {
    lastReport = millis();
    send(occupied ? "OCCUPIED" : "AVAILABLE");
  }

  // 3) Commands from the app (Bluetooth) or Serial Monitor
  readCommands(bt);
  readCommands(Serial);
  if (cmd.length() > 0 && millis() - lastCmdByte > 100) {
    handleCommand(cmd);
    cmd = "";
  }

  // 4) Close the gate automatically after OPEN_TIME_MS
  if (gateIsOpen && millis() - gateOpenedAt >= OPEN_TIME_MS) {
    closeGate();
    updateLcd();
  }
}
