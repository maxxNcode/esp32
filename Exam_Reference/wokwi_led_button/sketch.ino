// LED + button on a breadboard (Arduino Uno)
// Pin 13 -> 220 ohm resistor -> LED long leg (+); LED short leg (-) -> GND
// Pin 2  -> button -> GND   (INPUT_PULLUP: pressed = LOW)

const int LED = 13;
const int BUTTON = 2;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BUTTON) == LOW) {   // pressed
    digitalWrite(LED, HIGH);
    Serial.println("Button pressed - LED ON");
  } else {
    digitalWrite(LED, LOW);
  }
  delay(50);
}
