/*
  Problem 14 - Mobile-Controlled Garage Door (Arduino Uno + HC-05)

  - HC-05 Bluetooth: receives OPEN / CLOSE commands from the phone (MIT App Inventor app)
  - Servo: the garage door (0 = closed, 90 = open), moves slowly like a real door
  - Ultrasonic: vehicle closer than VEHICLE_CM = vehicle in the protected area
      * CLOSE is refused while a vehicle is detected
      * if a vehicle appears while closing, the door stops and opens again
  - LED: ON = door OPEN (or moving), OFF = door CLOSED
  - I2C LCD: door status + vehicle + distance
  - Buzzer: beeps while the door is opening

  Messages sent to the phone (one per line):
      DOOR OPENING / DOOR OPEN / DOOR CLOSING / DOOR CLOSED   (on change and every 2 s)
      VEHICLE: YES / VEHICLE: NO                              (on change and every 2 s)
      BLOCKED - VEHICLE DETECTED                              (CLOSE refused / closing stopped)

  Board: Arduino Uno.  Library: LiquidCrystal I2C (Frank de Brabander).
  Servo, SoftwareSerial and Wire are built in.
*/

#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pins ----------
const int BUZZER_PIN = 5;    // D5  -> buzzer +
const int LED_PIN    = 6;    // D6  -> LED long leg (PWM, see LED_LEVEL)
const int TRIG_PIN   = 7;    // D7  -> HC-SR04 Trig
const int ECHO_PIN   = 8;    // D8  -> HC-SR04 Echo
const int SERVO_PIN  = 9;    // D9  -> servo signal (orange)
const int BT_RX_PIN  = 10;   // D10 <- HC-05 TXD
const int BT_TX_PIN  = 11;   // D11 -> HC-05 RXD
const int BOARD_LED  = 13;   // built-in "L" LED, copies the door LED
// LCD: SDA = A4, SCL = A5

// ---------- Settings ----------
const int VEHICLE_CM  = 15;  // closer than this = vehicle in the protected area
const int DOOR_CLOSED = 0;   // servo angle when closed
const int DOOR_OPEN   = 90;  // servo angle when open
const unsigned long STEP_MS = 20;   // 1 degree every 20 ms -> about 1.8 s to open/close
const int LED_LEVEL   = 40;  // 0-255. Low PWM level because there is no LED resistor.
                             // With a 220 ohm resistor you can use 255.

SoftwareSerial bt(BT_RX_PIN, BT_TX_PIN);
Servo door;
LiquidCrystal_I2C lcd(0x27, 16, 2);   // if the LCD shows nothing, try 0x3F

enum DoorState { CLOSED, OPENING, OPEN, CLOSING };
DoorState state = CLOSED;
int angle = DOOR_CLOSED;

bool vehicle = false;
long distanceCm = 999;
String lcdMessage = "";
unsigned long messageUntil = 0;
unsigned long lastStep = 0, lastRead = 0, lastReport = 0, lastLcd = 0, lastBeep = 0;
bool beepOn = false;
String cmd;
unsigned long lastCmdByte = 0;

// ---------- Helpers ----------
const char *doorText() {
  switch (state) {
    case OPENING: return "DOOR OPENING";
    case OPEN:    return "DOOR OPEN";
    case CLOSING: return "DOOR CLOSING";
    default:      return "DOOR CLOSED";
  }
}

void send(const String &msg) {         // to the phone (HC-05) and the Serial Monitor
  bt.println(msg);
  Serial.println(msg);
}

void setLed(bool on) {
  analogWrite(LED_PIN, on ? LED_LEVEL : 0);
  digitalWrite(BOARD_LED, on ? HIGH : LOW);
}

void showMessage(const String &msg) {  // shown on LCD line 2 for 2 seconds
  lcdMessage = msg;
  messageUntil = millis() + 2000;
}

long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);   // 30 ms timeout
  if (duration == 0) return 999;                    // no echo = nothing there
  return duration * 0.034 / 2;
}

void updateLcd() {
  lcd.setCursor(0, 0);
  switch (state) {
    case OPENING: lcd.print("Door: OPENING..."); break;
    case OPEN:    lcd.print("Door: OPEN      "); break;
    case CLOSING: lcd.print("Door: CLOSING..."); break;
    default:      lcd.print("Door: CLOSED    "); break;
  }
  lcd.setCursor(0, 1);
  String line;
  if (millis() < messageUntil) {
    line = lcdMessage;
  } else {
    line = vehicle ? "Car: YES" : "Car: NO ";
    String d = (distanceCm >= 999) ? String("--cm") : String(distanceCm) + "cm";
    while (d.length() < 8) d = " " + d;
    line += d;
  }
  while (line.length() < 16) line += " ";
  lcd.print(line.substring(0, 16));
}

// ---------- Door actions ----------
void startOpening() {
  state = OPENING;
  setLed(true);                          // LED ON = door open / not closed
  send(doorText());
}

void startClosing() {
  state = CLOSING;
  noTone(BUZZER_PIN);                    // buzzer only sounds while opening
  beepOn = false;
  send(doorText());
}

void blocked() {
  send("BLOCKED - VEHICLE DETECTED");
  showMessage("BLOCKED: Car in!");
}

void handleCommand(String c) {
  c.trim();
  c.toUpperCase();
  if (c.length() == 0) return;
  Serial.println("Command: " + c);

  if (c == "OPEN" || c == "O" || c == "1") {
    if (state == CLOSED || state == CLOSING) startOpening();
    else send(doorText());                       // already open / opening
  } else if (c == "CLOSE" || c == "C" || c == "0") {
    if (vehicle) {
      blocked();                                 // never close on a vehicle
    } else if (state == OPEN || state == OPENING) {
      startClosing();
    } else {
      send(doorText());                          // already closed / closing
    }
  } else if (c == "STATUS") {
    send(doorText());
    send(vehicle ? "VEHICLE: YES" : "VEHICLE: NO");
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
  Serial.begin(9600);
  bt.begin(9600);                        // HC-05 default baud rate

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setLed(false);

  door.attach(SERVO_PIN);
  door.write(DOOR_CLOSED);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Garage Door");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

  Serial.println("Ready. Type OPEN or CLOSE (or use the app via HC-05).");
}

void loop() {
  unsigned long now = millis();

  // 1) Vehicle detection every 100 ms
  if (now - lastRead >= 100) {
    lastRead = now;
    distanceCm = readDistanceCm();
    bool nowVehicle = (distanceCm < VEHICLE_CM);
    if (nowVehicle != vehicle) {          // tell the phone at once when it changes
      vehicle = nowVehicle;
      send(vehicle ? "VEHICLE: YES" : "VEHICLE: NO");
    }
  }

  // 2) Move the door one step at a time
  if (now - lastStep >= STEP_MS) {
    lastStep = now;
    if (state == OPENING) {
      angle++;
      door.write(angle);
      if (angle >= DOOR_OPEN) {
        angle = DOOR_OPEN;
        state = OPEN;
        noTone(BUZZER_PIN);
        beepOn = false;
        send(doorText());
      }
    } else if (state == CLOSING) {
      if (vehicle) {                 // safety: vehicle appeared -> stop and open again
        blocked();
        startOpening();
      } else {
        angle--;
        door.write(angle);
        if (angle <= DOOR_CLOSED) {
          angle = DOOR_CLOSED;
          state = CLOSED;
          setLed(false);             // LED OFF = door closed
          send(doorText());
        }
      }
    }
  }

  // 3) Buzzer beeps while opening (150 ms on / 150 ms off)
  if (state == OPENING && now - lastBeep >= 150) {
    lastBeep = now;
    beepOn = !beepOn;
    if (beepOn) tone(BUZZER_PIN, 2000);
    else noTone(BUZZER_PIN);
  }

  // 4) Commands from the phone (HC-05) or Serial Monitor
  readCommands(bt);
  readCommands(Serial);
  if (cmd.length() > 0 && now - lastCmdByte > 100) {
    handleCommand(cmd);
    cmd = "";
  }

  // 5) LCD refresh and status report every 2 s
  if (now - lastLcd >= 250) {
    lastLcd = now;
    updateLcd();
  }
  if (now - lastReport >= 2000) {
    lastReport = now;
    send(doorText());
    send(vehicle ? "VEHICLE: YES" : "VEHICLE: NO");
  }
}
