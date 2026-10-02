# Activity 7: ESP32 + Firebase + MIT App Inventor (Android)

```
 MIT App Inventor app  --->  Firebase Realtime DB  --->  ESP32  --->  LED1-4 + Servo
   (Android phone)           LED1: 0   LED4: 0
                             LED2: 0   ALL_LED: 0
                             LED3: 0   SERVO: 0
```

You need: a Google account, a laptop with Arduino IDE, an Android phone, an ESP32,
4 LEDs + 4 × 220 Ω resistors, 1 servo (SG90), a breadboard, jumper wires, and 2.4 GHz Wi-Fi
(a phone hotspot works).

---

## Part 1: Firebase

### 1.1 Create the project
1. Go to https://console.firebase.google.com and sign in with your Google account.
2. Click **Create a project** (or **Get started with a Firebase project** / **Add project**).
3. **Project name:** `activity7`. Tick **I accept the Firebase terms** (and the confirm box, if shown), then **Continue**.
4. **AI assistance / Gemini in Firebase:** you can turn it off. Click **Continue**.
5. **Google Analytics:** turn it **off**. Click **Create project**.
6. Wait for "Your Firebase project is ready", then click **Continue**.

### 1.2 Register an Android app ("choose iOS / Android / Web")
The Project Overview page asks you to **add Firebase to your app** with icons for iOS, Android, Web, Unity and Flutter.

> MIT App Inventor connects with the **database URL + secret**, so this step is **optional**.
> If your teacher wants it, do it as below. Otherwise skip to 1.3.

1. Click the **Android** icon.
2. **Android package name:** `appinventor.ai_YOURUSERNAME.Activity7`
   (YOURUSERNAME = the part of your Gmail before `@`, for example `appinventor.ai_juan.Activity7`).
   The name can't contain dots or dashes. If your username has them, change them to `_`.
3. **App nickname:** `Activity7`. Leave **SHA-1** empty. Click **Register app**.
4. **Download google-services.json:** you can download it, but App Inventor **does not use it**. Click **Next**.
5. **Add Firebase SDK:** this is for Android Studio only. Click **Next**, then **Continue to console**.

### 1.3 Create the Realtime Database
1. In the left menu, open **Build** (in newer consoles, **Databases & Storage**) → **Realtime Database**.
   Use **Realtime Database**, not *Firestore Database*.
2. Click **Create Database**.
3. **Database location:** **Singapore (asia-southeast1)** or **United States (us-central1)**. Click **Next**.
4. **Security rules:** choose **Start in test mode**. Click **Enable**.
5. At the top of the **Data** tab you'll see the URL. Copy it into a notepad. It looks like:
   `https://activity7-xxxx-default-rtdb.asia-southeast1.firebasedatabase.app/`
   This is your **URL**.

### 1.4 Add the starting data
1. On your laptop, make a text file named `data.json` containing:
   ```json
   { "LED1": 0, "LED2": 0, "LED3": 0, "LED4": 0, "ALL_LED": 0, "SERVO": 0 }
   ```
2. In the **Data** tab, click **⋮** (top right of the data box), choose **Import JSON**, pick `data.json`, and click **Import**.
3. You should now see the 6 keys, all set to `0`.

### 1.5 Get the database secret
1. Click ⚙ (next to **Project Overview**) and open **Project settings**.
2. Open the **Service accounts** tab, then **Database secrets** (left side).
3. Hover over the secret, click **Show**, and copy it into your notepad. This is your **SECRET**.
   (Google labels it "deprecated", but it still works.)

### 1.6 Rules (after 30 days)
Test mode stops working after 30 days. If you get "permission denied" later, go to **Realtime Database → Rules**, paste this, and click **Publish**:
```json
{ "rules": { ".read": true, ".write": true } }
```

---

## Part 2: MIT App Inventor app (Android)

### 2.1 Set up your Android phone
1. On the phone, open the **Play Store** and install **MIT AI2 Companion**.
2. Connect the phone and the laptop to the **same Wi-Fi**.

### 2.2 Import the project
1. On the laptop, go to https://ai2.appinventor.mit.edu and sign in with your Google account.
   On first use, accept the **Terms of Service** and close the welcome window.
2. Download **`Activity7.aia`** from this folder.
3. Choose **Projects → Import project (.aia) from my computer → Choose File →** `Activity7.aia` **→ OK**.
4. The project opens in the **Designer**. You'll see a title, 13 buttons and a status label.

### 2.3 Connect it to your Firebase
1. Under the phone preview, in **Non-visible components**, click **FirebaseDB1**.
2. In the **Properties** panel on the right:
   - **FirebaseURL**: click the box, choose **Use Custom**, paste your **URL**, and click **OK**.
   - **FirebaseToken**: delete what's there and paste your **SECRET**.
   - **ProjectBucket**: delete everything so it's **empty**.
   - **Persist**: leave it unticked.
3. Optional: open **Blocks** (top right) to see the blocks. Each button has
   `call FirebaseDB1.StoreValue  tag "LED1"  valueToStore 1`.

### 2.4 Test with AI Companion
1. On the website, choose **Connect → AI Companion**. A QR code appears.
2. On the phone, open **MIT AI2 Companion** and tap **scan QR code** (or type the 6-letter code), then **connect with code**.
3. The app appears on the phone. Press **LED1 ON**.
4. In the Firebase console, `LED1` should turn to `1` (it flashes yellow or green). ✅
   If not, the status label at the bottom of the app shows the Firebase error.

### 2.5 Install it as a real Android app (.apk)
1. Choose **Build → Android App (.apk)**. Wait for it to finish (1–2 minutes).
2. Either scan the QR code with the **Companion** app's scanner (or any QR app), or click **Download .apk** and copy it to the phone.
3. Open the file on the phone. If Android blocks it, tap **Settings → Allow from this source**, go back, and tap **Install**.
4. If **Play Protect** warns you, tap **More details → Install anyway**.
5. Open **Activity7** from the phone's app list.

---

## Part 3: ESP32

### 3.1 Install the Arduino IDE and the ESP32 board
1. Download **Arduino IDE 2** from https://www.arduino.cc/en/software and install it.
2. Go to **File → Preferences → Additional boards manager URLs**, paste
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`, and click **OK**.
3. In **Tools → Board → Boards Manager**, search for `esp32` and install **esp32 by Espressif Systems**.
4. In **Tools → Manage Libraries**, search and install:
   - **Firebase Arduino Client Library for ESP8266 and ESP32** (by **Mobizt**). Click **Install all** if asked.
   - **ESP32Servo** (by Kevin Harrington)
5. If the laptop doesn't see the ESP32 as a port, install the USB driver:
   **CP210x** (https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) or **CH340**, depending on the chip on your board.

### 3.2 Wiring

| Part | ESP32 pin |
|------|-----------|
| LED1: long leg (+) → 220 Ω resistor | GPIO 16 |
| LED2: long leg (+) → 220 Ω resistor | GPIO 17 |
| LED3: long leg (+) → 220 Ω resistor | GPIO 18 |
| LED4: long leg (+) → 220 Ω resistor | GPIO 19 |
| All LED short legs (−) | GND |
| Servo **orange/yellow** (signal) | GPIO 13 |
| Servo **red** (+) | 5V / VIN |
| Servo **brown/black** (−) | GND |

### 3.3 Edit and upload the code
1. Open `Activity7_Firebase_LED_Servo/Activity7_Firebase_LED_Servo.ino`.
2. Change the top of the file:
   ```cpp
   #define WIFI_SSID       "your wifi name"
   #define WIFI_PASSWORD   "your wifi password"
   #define DATABASE_URL    "activity7-xxxx-default-rtdb.asia-southeast1.firebasedatabase.app"
   #define DATABASE_SECRET "your secret"
   ```
   For `DATABASE_URL`, take your URL and remove `https://` and the `/` at the end.
3. Plug in the ESP32 with a **data** USB cable (some cables only charge).
4. Under **Tools → Board → esp32**, choose **ESP32 Dev Module**.
5. Under **Tools → Port**, choose the COM port that appeared (for example COM5).
6. Click **Upload (→)**. If it gets stuck on `Connecting.....`, **hold the BOOT button** until uploading starts.
7. Open **Tools → Serial Monitor** and set it to **115200 baud**. Press the ESP32's **EN/RST** button. You should see:
   ```
   Connecting to WiFi....
   Connected, IP: 192.168.x.x
   LED1 -> OFF
   ...
   ```

---

## Part 4: Test everything

| Press in app | Firebase | ESP32 |
|--------------|----------|-------|
| LED1 ON / OFF … LED4 ON / OFF | `LEDx` = 1 / 0 | that LED turns on / off |
| ALL LED ON | `ALL_LED` = 1, LED1–4 = 1 | all 4 LEDs on |
| ALL LED OFF | `ALL_LED` = 0, LED1–4 = 0 | all 4 LEDs off |
| SERVO 0 / 90 / 180 | `SERVO` = 0 / 90 / 180 | servo turns to that angle |

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| No COM port in Arduino | Install the CP210x/CH340 driver, or try another USB cable |
| Upload stuck on `Connecting.....` | Hold **BOOT** while uploading |
| Stuck on `Connecting to WiFi....` | Use **2.4 GHz** Wi-Fi and check the name and password (they're case-sensitive) |
| `Firebase read failed: permission denied` | The secret is wrong, or test mode has expired (see 1.6) |
| `Firebase read failed: ... host` / `connection refused` | `DATABASE_URL` still has `https://` or a trailing `/` |
| App values appear in a sub-folder in Firebase | Clear **ProjectBucket** in FirebaseDB1 |
| AI Companion won't connect | Put the phone and laptop on the same Wi-Fi, or use **Connect → USB** |
| Android won't install the .apk | Allow **Install unknown apps** for your browser or file manager |
| ESP32 restarts when the servo moves | Power the servo from a separate 5V supply and connect the grounds |
| An LED doesn't light | Flip the LED (the long leg goes to the resistor/GPIO side) |
| ALL LED ON does nothing | Press ALL LED OFF first. It only reacts when the value changes. |

To regenerate the .aia: `python3 make_aia.py`.
