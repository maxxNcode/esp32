# Hands-on Exam Reference: Arduino Uno & ESP32

How to wire common components, plus a code template for each.
Find the components in your problem, wire them as shown, copy the code, and combine.

---

## 0. The basics (read this first)

### Breadboard
```
   +  -                                   +  -     <- power rails (run along the long side)
   |  |   a b c d e     f g h i j         |  |
   |  |   o o o o o  |  o o o o o         |  |   row 1: a-e are connected together,
   |  |   o o o o o  |  o o o o o         |  |          f-j are connected together
   |  |   o o o o o  |  o o o o o         |  |   the middle gap SEPARATES the two sides
```
- Each **numbered row of 5 holes** (a–e, or f–j) is **one connection**.
- The **long rails** on the sides: connect **red (+)** to 5V (Uno) or 3.3V (ESP32), and **blue (−)** to **GND**.
  On long breadboards the rails are sometimes split in the middle. Bridge them with a wire.
- Two legs of one component must **never** be in the same row, or you short it.

### The 3 connections every component needs
1. **Power (VCC / + / VIN)**: 5V on Uno. On ESP32, 3.3V, or VIN/5V for 5V modules like servo, LCD and ultrasonic.
2. **Ground (GND / − / G)**: always connect it. **All GNDs must be connected together**.
3. **Signal (S / OUT / IN / DATA / SIG)**: goes to a pin you choose, and the **same pin number goes in the code**.

### Which pins to use

| | Arduino Uno | ESP32 DevKit |
|---|---|---|
| Logic voltage | 5V | **3.3V** |
| Digital pins (LED, button, buzzer, relay) | 2–13 (avoid 0, 1) | 4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 |
| PWM (brightness, servo) | ~3, ~5, ~6, ~9, ~10, ~11 | any output pin above |
| Analog input (pot, LDR, sensors) | A0–A5 (0–1023) | **32, 33, 34, 35, 36(VP), 39(VN)** (0–4095) |
| I2C (LCD, OLED) | SDA = **A4**, SCL = **A5** | SDA = **21**, SCL = **22** |
| Don't use | 0, 1 (USB serial) | 6–11 (flash), and avoid 0, 2, 12, 15 for inputs (boot pins) |
| Input-only pins | – | 34, 35, 36, 39 (sensors only, no LEDs) |
| Built-in LED | 13 | 2 |

### Arduino IDE settings
- **Uno:** Tools → Board → **Arduino Uno**, Port → COMx.
- **ESP32:** Tools → Board → **ESP32 Dev Module**, Port → COMx. If the upload gets stuck at `Connecting...`, hold **BOOT**.
- **Serial Monitor:** use the same baud as `Serial.begin(...)`. Below it's always **9600** (Uno) / **115200** (ESP32).

### Libraries (Tools → Manage Libraries)

| Component | Library to install |
|-----------|--------------------|
| Servo on Uno | *Servo* (built in) |
| Servo on ESP32 | **ESP32Servo** |
| DHT11 / DHT22 | **DHT sensor library** (Adafruit) + **Adafruit Unified Sensor** |
| LCD 16x2 I2C | **LiquidCrystal I2C** (Frank de Brabander) |
| OLED SSD1306 | **Adafruit SSD1306** + **Adafruit GFX** |

---

## 1. LED

**Legs:** **long leg = + (anode)**, short leg / flat side = − (cathode).
Always use a **220 Ω resistor** (red-red-brown), or 330 Ω.

```
Pin ──── 220Ω ──── LED long leg (+)
                   LED short leg (−) ──── GND
```

| LED | Uno | ESP32 |
|-----|-----|-------|
| + (through resistor) | D13 (or any 2–13) | GPIO 2 / 16 / 17 / 18 / 19 |
| − | GND | GND |

```cpp
const int LED = 13;          // ESP32: 16

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, HIGH);   // ON
  delay(500);
  digitalWrite(LED, LOW);    // OFF
  delay(500);
}
```
**Brightness (fade):** `analogWrite(LED, 0..255);` (Uno: use a ~ pin).

### Multiple LEDs (traffic light)
Red → D8 / GPIO 16, Yellow → D9 / GPIO 17, Green → D10 / GPIO 18. Each goes through its own 220 Ω resistor, and all short legs go to GND.
```cpp
const int RED = 8, YELLOW = 9, GREEN = 10;   // ESP32: 16, 17, 18

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
}

void light(int r, int y, int g, int ms) {
  digitalWrite(RED, r);
  digitalWrite(YELLOW, y);
  digitalWrite(GREEN, g);
  delay(ms);
}

void loop() {
  light(HIGH, LOW, LOW, 5000);   // red 5 s
  light(LOW, LOW, HIGH, 5000);   // green 5 s
  light(LOW, HIGH, LOW, 2000);   // yellow 2 s
}
```

---

## 2. Push button

A 4-leg button is connected in pairs. Place it **across the middle gap**, then use two **diagonal** legs.
Use `INPUT_PULLUP`, so no resistor is needed: **pressed = LOW**.

```
Pin ──── button leg 1
         button leg 2 (diagonal) ──── GND
```

| Button | Uno | ESP32 |
|--------|-----|-------|
| one leg | D2 | GPIO 4 |
| diagonal leg | GND | GND |

```cpp
const int BUTTON = 2, LED = 13;     // ESP32: 4, 16

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(BUTTON) == LOW) {   // pressed
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
```

**Toggle** (press once = ON, press again = OFF):
```cpp
const int BUTTON = 2, LED = 13;     // ESP32: 4, 16
bool ledOn = false;
int lastState = HIGH;

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  int state = digitalRead(BUTTON);
  if (lastState == HIGH && state == LOW) {   // just pressed
    ledOn = !ledOn;
    digitalWrite(LED, ledOn);
    delay(50);                               // debounce
  }
  lastState = state;
}
```

---

## 3. Buzzer

**+ leg (longer, or marked "+") → pin, − → GND.**
- **Active buzzer** (sealed bottom): beeps with `digitalWrite`.
- **Passive buzzer** (green board visible underneath): needs `tone()`.
- **3-pin module** (S, +, −): S → pin, + → 5V/3.3V, − → GND.

| Buzzer | Uno | ESP32 |
|--------|-----|-------|
| + / S | D8 | GPIO 25 |
| − | GND | GND |

```cpp
const int BUZZER = 8;              // ESP32: 25

void setup() {
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  tone(BUZZER, 1000);    // 1000 Hz (works for both types)
  delay(300);
  noTone(BUZZER);
  delay(700);
  // active buzzer only: digitalWrite(BUZZER, HIGH / LOW);
}
```

---

## 4. Potentiometer (knob)

3 legs: **outer legs = power and GND, middle leg = signal.**

| Pot | Uno | ESP32 |
|-----|-----|-------|
| left leg | 5V | 3.3V |
| **middle** | **A0** | **GPIO 34** |
| right leg | GND | GND |

```cpp
const int POT = A0;     // ESP32: 34
const int LED = 9;      // PWM pin. ESP32: 16

void setup() {
  Serial.begin(9600);   // ESP32: 115200
  pinMode(LED, OUTPUT);
}

void loop() {
  int value = analogRead(POT);                     // Uno 0-1023, ESP32 0-4095
  int brightness = map(value, 0, 1023, 0, 255);    // ESP32: map(value, 0, 4095, 0, 255)
  analogWrite(LED, brightness);
  Serial.println(value);
  delay(50);
}
```

---

## 5. LDR (light sensor)

**Module (3 or 4 pins):** VCC → 5V/3.3V, GND → GND, **AO** → analog pin (DO = on/off signal, optional).
**Bare LDR + 10 kΩ resistor (voltage divider):**
```
5V (ESP32: 3.3V) ──── LDR ────┬──── A0 (ESP32: GPIO 34)
                              └──── 10kΩ ──── GND
```

```cpp
const int LDR = A0, LED = 13;   // ESP32: 34, 16
const int DARK = 300;           // check Serial Monitor, then adjust

void setup() {
  Serial.begin(9600);           // ESP32: 115200
  pinMode(LED, OUTPUT);
}

void loop() {
  int light = analogRead(LDR);
  Serial.println(light);
  if (light < DARK) {
    digitalWrite(LED, HIGH);    // dark: night light ON
  } else {
    digitalWrite(LED, LOW);
  }
  delay(200);
}
```
With a **module** the reading is often **reversed** (dark = high number). Read the Serial Monitor and flip `<` to `>` if needed.

---

## 6. Servo motor (SG90)

| Wire color | Meaning | Uno | ESP32 |
|------------|---------|-----|-------|
| **Brown** / black | GND | GND | GND |
| **Red** | power | 5V | **VIN** (5V) |
| **Orange** / yellow | signal | D9 | GPIO 13 |

```cpp
#include <Servo.h>              // ESP32: #include <ESP32Servo.h>
Servo myServo;

void setup() {
  myServo.attach(9);            // ESP32: myServo.attach(13);
}

void loop() {
  myServo.write(0);
  delay(1000);
  myServo.write(90);
  delay(1000);
  myServo.write(180);
  delay(1000);
}
```
If the board **restarts** when the servo moves, give the servo its own 5V supply and connect the GNDs together.

---

## 7. Ultrasonic sensor (HC-SR04): distance

| HC-SR04 | Uno | ESP32 |
|---------|-----|-------|
| VCC | 5V | VIN (5V) |
| Trig | D9 | GPIO 5 |
| Echo | D10 | GPIO 18 (through a 1k/2k divider, see below) |
| GND | GND | GND |

ESP32 only: Echo outputs 5V. To be safe, use **Echo → 1 kΩ → GPIO 18, and GPIO 18 → 2 kΩ → GND**.
Many class setups connect Echo directly and it works, but the divider protects the pin.

```cpp
const int TRIG = 9, ECHO = 10;    // ESP32: 5, 18

void setup() {
  Serial.begin(9600);             // ESP32: 115200
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

long readCM() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000);   // timeout 30 ms
  return duration * 0.034 / 2;                  // cm (0 = nothing detected)
}

void loop() {
  long cm = readCM();
  Serial.print("Distance: ");
  Serial.print(cm);
  Serial.println(" cm");
  delay(200);
}
```

---

## 8. DHT11 / DHT22: temperature and humidity

| DHT (3-pin module) | Uno | ESP32 |
|--------------------|-----|-------|
| + / VCC | 5V | 3.3V |
| OUT / DATA / S | D2 | GPIO 4 |
| − / GND | GND | GND |

For a **bare 4-pin DHT11** (blue box, pins facing you, left to right): 1 = VCC, 2 = DATA (+ 10 kΩ to VCC), 3 = unused, 4 = GND.

```cpp
#include <DHT.h>
#define DHTPIN 2          // ESP32: 4
#define DHTTYPE DHT11     // or DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);     // ESP32: 115200
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();   // °C
  float h = dht.readHumidity();      // %
  if (isnan(t) || isnan(h)) {
    Serial.println("DHT read failed - check wiring");
  } else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print(" C   Humidity: ");
    Serial.print(h);
    Serial.println(" %");
  }
  delay(2000);            // DHT11 needs ~2 s between reads
}
```

---

## 9. LCD 16x2 with I2C backpack (4 pins)

| LCD pin | Uno | ESP32 |
|---------|-----|-------|
| GND | GND | GND |
| VCC | 5V | VIN (5V) |
| SDA | **A4** | **GPIO 21** |
| SCL | **A5** | **GPIO 22** |

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);   // if blank, try 0x3F

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);   // column 0, row 0
  lcd.print("Hello!");
}

void loop() {
  lcd.setCursor(0, 1);   // row 1 (second line)
  lcd.print("Time: ");
  lcd.print(millis() / 1000);
  lcd.print("s   ");      // trailing spaces clear old characters
  delay(500);
}
```
- **Backlight on but no text:** turn the **blue contrast screw** on the back with a small screwdriver.
- **Nothing works:** try address `0x3F` instead of `0x27`.

---

## 10. Relay module (to switch a lamp or fan)

| Relay | Uno | ESP32 |
|-------|-----|-------|
| VCC / + | 5V | VIN (5V) |
| GND / − | GND | GND |
| IN / S | D7 | GPIO 26 |

Load side (screw terminals): **COM** + **NO** (normally open) = ON when the relay clicks.
⚠️ Don't touch mains (220V) wiring unless your instructor sets it up.

```cpp
const int RELAY = 7;     // ESP32: 26

void setup() {
  pinMode(RELAY, OUTPUT);
}

void loop() {
  digitalWrite(RELAY, HIGH);   // many modules are "active LOW":
  delay(2000);                 // if ON/OFF are swapped, swap HIGH and LOW
  digitalWrite(RELAY, LOW);
  delay(2000);
}
```

---

## 11. PIR motion sensor (HC-SR501)

| PIR | Uno | ESP32 |
|-----|-----|-------|
| VCC | 5V | VIN (5V) |
| OUT | D2 | GPIO 27 |
| GND | GND | GND |

```cpp
const int PIR = 2, LED = 13;   // ESP32: 27, 16

void setup() {
  pinMode(PIR, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, digitalRead(PIR));   // motion = HIGH = LED ON
}
```
It needs about **30–60 s to warm up** after power-on. The two orange knobs set sensitivity and on-time.

---

## 12. RGB LED (4 legs)

**Longest leg = common.** For common **cathode**, it goes to **GND**; for common **anode**, it goes to **5V/3.3V**.
The other 3 legs are R, G, B, **each through a 220 Ω resistor**.

| RGB leg | Uno | ESP32 |
|---------|-----|-------|
| R | ~9 | GPIO 25 |
| longest (common) | GND | GND |
| G | ~10 | GPIO 26 |
| B | ~11 | GPIO 27 |

```cpp
const int R = 9, G = 10, B = 11;   // ESP32: 25, 26, 27

void setup() {
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
}

void color(int r, int g, int b) {   // 0-255 each
  analogWrite(R, r);
  analogWrite(G, g);
  analogWrite(B, b);
}

void loop() {
  color(255, 0, 0);   delay(1000);  // red
  color(0, 255, 0);   delay(1000);  // green
  color(0, 0, 255);   delay(1000);  // blue
  color(255, 255, 0); delay(1000);  // yellow
}
```
For a common **anode** LED, the colors are inverted: use `255 - value`.

---

## 13. Analog sensor modules (soil moisture, rain, gas MQ-2, sound, flame, IR obstacle)

They all have the same pins: **VCC, GND, AO (analog), DO (digital on/off)**.

| Module | Uno | ESP32 |
|--------|-----|-------|
| VCC | 5V | 3.3V (MQ gas: VIN 5V) |
| GND | GND | GND |
| AO | A0 | GPIO 34 |
| DO (optional) | D2 | GPIO 27 |

```cpp
const int SENSOR = A0, LED = 13;   // ESP32: 34, 16
const int LIMIT = 500;             // read Serial Monitor, then adjust

void setup() {
  Serial.begin(9600);              // ESP32: 115200
  pinMode(LED, OUTPUT);
}

void loop() {
  int v = analogRead(SENSOR);
  Serial.println(v);
  digitalWrite(LED, v > LIMIT ? HIGH : LOW);
  delay(200);
}
```
The **DO** pin gives HIGH/LOW, and the blue screw on the module sets the trigger level. Read it with `digitalRead()`.

---

## 14. How to combine (typical exam problems)

Every program follows the same pattern: **read inputs → decide with `if` → control outputs.**

### Example: "If an object is closer than 10 cm, turn on the red LED and buzzer; otherwise turn on the green LED"
Wiring: ultrasonic (section 7) + red LED D12 + green LED D11 + buzzer D8.
ESP32: red 16, green 17, buzzer 25, Trig 5, Echo 18.
```cpp
const int TRIG = 9, ECHO = 10, RED = 12, GREEN = 11, BUZZER = 8;
// ESP32: TRIG = 5, ECHO = 18, RED = 16, GREEN = 17, BUZZER = 25

void setup() {
  Serial.begin(9600);   // ESP32: 115200
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

long readCM() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, 30000) * 0.034 / 2;
}

void loop() {
  long cm = readCM();
  Serial.println(cm);

  if (cm > 0 && cm < 10) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    tone(BUZZER, 1000);
  } else {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    noTone(BUZZER);
  }
  delay(100);
}
```

### Example: "Show temperature on the LCD; if above 30 °C turn on the fan (relay)"
Wiring: DHT (section 8) + LCD (section 9) + relay (section 10).
```cpp
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

DHT dht(2, DHT11);                    // ESP32: DHT dht(4, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int RELAY = 7;                  // ESP32: 26

void setup() {
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(RELAY, OUTPUT);
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t, 1);
  lcd.print(" C   ");
  lcd.setCursor(0, 1);
  lcd.print("Hum:  ");
  lcd.print(h, 0);
  lcd.print(" %   ");

  digitalWrite(RELAY, t > 30 ? HIGH : LOW);
  delay(2000);
}
```

---

## 15. Exam checklist

1. **Unplug USB before wiring.** Wire everything, double-check, then plug in.
2. Connect **GND first**, then power, then signal.
3. LEDs: **long leg towards the pin**, with a resistor.
4. The **pin number in the code = the pin you wired.**
5. Choose the right **board and port** before uploading.
6. Use **`Serial.println()`** to see sensor values, and pick thresholds from what you see.
7. **Something doesn't work?** Test one part at a time: first make the LED blink, then add a sensor.

| Problem | Fix |
|---------|-----|
| `'xxx' was not declared in this scope` | A typo, or a missing library or `#include` |
| `xxx.h: No such file or directory` | Install the library (Tools → Manage Libraries) |
| Upload fails / no port | Use a data cable and pick the right port. ESP32: hold BOOT. |
| Serial Monitor shows garbage | Wrong baud rate; match `Serial.begin()` |
| LED doesn't light | Flipped LED, no resistor connection, wrong pin, or GND missing |
| Sensor reads 0 or nonsense | Check VCC/GND. Analog sensors on ESP32 must use pins 32–39. |
| ESP32 resets randomly | Servo/relay drawing too much power. Use a separate 5V supply and connect the grounds. |
