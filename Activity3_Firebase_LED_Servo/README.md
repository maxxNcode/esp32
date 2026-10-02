# Activity 3: ESP32 + Firebase + MIT App Inventor

```
 MIT App Inventor app  --->  Firebase Realtime DB  --->  ESP32  --->  LED1-4 + Servo
   (buttons)                 LED1: 0   LED4: 0
                             LED2: 0   ALL_LED: 0
                             LED3: 0   SERVO: 0
```

The app never talks to the ESP32 directly. Each button writes a value to
Firebase, and the ESP32 reads Firebase about 3 times a second.

## 1. Wiring

| Part   | ESP32 pin | Notes                                              |
|--------|-----------|----------------------------------------------------|
| LED1   | GPIO 16   | GPIO → 220–330 Ω resistor → LED long leg; short leg → GND |
| LED2   | GPIO 17   | same                                               |
| LED3   | GPIO 18   | same                                               |
| LED4   | GPIO 19   | same                                               |
| Servo signal (orange/yellow) | GPIO 13 |                                  |
| Servo + (red)   | 5V / VIN | use an external 5V supply if the ESP32 resets   |
| Servo − (brown) | GND      | must share GND with the ESP32                   |

## 2. Firebase setup

1. Go to https://console.firebase.google.com → **Add project**.
2. **Build → Realtime Database → Create database** → choose **Start in test mode**.
3. Copy the database URL, e.g. `https://activity3-1234-default-rtdb.firebaseio.com/`.
4. Get the secret: **⚙ Project settings → Service accounts → Database secrets → Show**.
5. Optional: add the starting data. Click **⋮ → Import JSON** and import:

```json
{ "LED1": 0, "LED2": 0, "LED3": 0, "LED4": 0, "ALL_LED": 0, "SERVO": 0 }
```

## 3. ESP32 code

1. Arduino IDE → Boards Manager → install **esp32 by Espressif Systems**. Select **ESP32 Dev Module**.
2. Library Manager → install:
   - **Firebase Arduino Client Library for ESP8266 and ESP32** (by Mobizt)
   - **ESP32Servo**
3. Open `Activity3_Firebase_LED_Servo.ino` and fill in:
   - `WIFI_SSID`, `WIFI_PASSWORD` (must be **2.4 GHz** Wi-Fi)
   - `DATABASE_URL`: the URL **without** `https://` and without the trailing `/`
   - `DATABASE_SECRET`: the secret from step 2.4
4. Upload, then open the Serial Monitor at **115200** baud.

## 4. MIT App Inventor app

### Fast way: import the ready-made project

1. Download `Activity3.aia` from this folder.
2. Go to https://ai2.appinventor.mit.edu → **Projects → Import project (.aia) from my computer** → choose `Activity3.aia`.
3. In the **Designer**, click **FirebaseDB1** (under Non-visible components) and set:
   - **FirebaseURL** → *Use Custom* → your database URL (e.g. `https://activity3-1234-default-rtdb.firebaseio.com/`)
   - **FirebaseToken** → your database secret
   - **ProjectBucket** → leave it empty
4. Done. All 13 buttons and their blocks are already set up. Connect with **AI Companion**, or use **Build → Android App (.apk)**.

(To regenerate the .aia, run `python3 make_aia.py`.)

The rest of this section shows how to build the same app by hand.

### Designer

- **Screen1**: 13 Buttons, as on the whiteboard. A `TableArrangement` (2 columns × 7 rows) keeps them tidy.

| Button name   | Text        |
|---------------|-------------|
| btnLed1On / btnLed1Off | LED1 ON / LED1 OFF |
| btnLed2On / btnLed2Off | LED2 ON / LED2 OFF |
| btnLed3On / btnLed3Off | LED3 ON / LED3 OFF |
| btnLed4On / btnLed4Off | LED4 ON / LED4 OFF |
| btnAllOn / btnAllOff   | ALL LED ON / ALL LED OFF |
| btnServo90 / btnServo180 / btnServo0 | SERVO 90 / SERVO 180 / SERVO 0 |

- **FirebaseDB1** (Palette → *Experimental* → FirebaseDB). Its properties:
  - **FirebaseURL**: your database URL, e.g. `https://activity3-1234-default-rtdb.firebaseio.com/`
  - **FirebaseToken**: the database secret from step 2.4
  - **ProjectBucket**: **clear it (leave empty)**. If you leave text here, the app writes
    to `/<bucket>/LED1`, and the ESP32 looks for `/LED1` and won't find it.

### Blocks

Every button uses the same block: `call FirebaseDB1.StoreValue tag ... valueToStore ...`

| When ... .Click | tag       | valueToStore |
|-----------------|-----------|--------------|
| btnLed1On       | `LED1`    | `1` |
| btnLed1Off      | `LED1`    | `0` |
| btnLed2On       | `LED2`    | `1` |
| btnLed2Off      | `LED2`    | `0` |
| btnLed3On       | `LED3`    | `1` |
| btnLed3Off      | `LED3`    | `0` |
| btnLed4On       | `LED4`    | `1` |
| btnLed4Off      | `LED4`    | `0` |
| btnAllOn        | `ALL_LED` | `1` |
| btnAllOff       | `ALL_LED` | `0` |
| btnServo90      | `SERVO`   | `90` |
| btnServo180     | `SERVO`   | `180` |
| btnServo0       | `SERVO`   | `0` |

Example (LED1 ON):

```
when btnLed1On.Click
do  call FirebaseDB1.StoreValue
        tag           "LED1"        <- text block
        valueToStore  1             <- number block (Math)
```

Use a **text** block for the tag and a **number** block from Math for the value.
The ESP32 also accepts text values ("1"), so either works.

**How ALL LED works:** the ESP32 acts when `ALL_LED` **changes**. It turns all 4 LEDs on or off
and writes the same value into LED1–LED4 in Firebase. After that, the single LED buttons still
work. If ALL LED ON doesn't respond, press ALL LED OFF first, because the value has to change.

## 5. Testing

1. Watch the Serial Monitor. It should print `Connected, IP: ...` and no `Firebase read failed`.
2. Open the Firebase console and change `LED1` to `1` by hand. LED1 should light up.
3. Run the app with **AI Companion** and press the buttons. The values in the console should change,
   and so should the LEDs and the servo.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Stuck at `Connecting to WiFi....` | Wrong SSID/password, or the network is 5 GHz only |
| `Firebase read failed: permission denied` | Wrong `DATABASE_SECRET`, or the rules aren't in test mode |
| `Firebase read failed: ... host` | `DATABASE_URL` must not include `https://` or the trailing `/` |
| The app changes values, but under another folder | Clear **ProjectBucket** in FirebaseDB1 |
| ESP32 resets or browns out when the servo moves | Power the servo from an external 5V supply and connect the grounds |
| LED stays off | Flip the LED (the long leg goes to the GPIO side) and check the resistor |
