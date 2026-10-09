#include "Arduino.h"
unsigned long g_ms=0; int g_in[64]; int g_out[64]; int g_pwm[64]; bool g_tone=false; int g_toneStarts=0; int g_analog=200;
float g_h=60.0f, g_t=27.0f;
SerialT Serial;
#include "sketch.cpp"
int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok:   %s\n",msg);}while(0)
void run(unsigned long ms){ for(unsigned long i=0;i<ms;i++){ g_ms++; loop(); } }
void show(){ printf("      LCD: [%s] [%s]\n", lcd.rows[0].c_str(), lcd.rows[1].c_str()); }
bool ledOn(){ return g_pwm[LED_PIN]>0 && g_out[BOARD_LED]==HIGH; }
void press(){ g_in[BUTTON_PIN]=LOW; run(3); g_in[BUTTON_PIN]=HIGH; run(3); g_in[BUTTON_PIN]=LOW; run(150); g_in[BUTTON_PIN]=HIGH; run(150); }
int main(){
  g_in[BUTTON_PIN]=HIGH;
  setup(); run(300); show();
  CHECK(lcd.rows[0]=="H:60% Light:81% " && lcd.rows[1]=="Status: OK      ", "bright (raw 200=81%) + 60% humidity -> OVERVIEW OK");
  CHECK(!ledOn() && !g_tone, "LED off, buzzer off");
  // getting dark
  g_analog=750; run(300); show();   // 27% light
  CHECK(ledOn() && g_pwm[LED_PIN]==LED_LEVEL, "light 27% (< 30%) -> LED ON at safe level");
  CHECK(lcd.rows[1]=="Status: DARK    " && !g_tone, "LCD Status: DARK, no buzzer for darkness");
  g_analog=690; run(300);           // 33%: inside hysteresis band
  CHECK(ledOn(), "33% (between 30 and 35) -> LED stays ON (no flicker)");
  g_analog=600; run(300);           // 42%
  CHECK(!ledOn(), "42% -> bright again, LED OFF");
  // humidity falls below 40
  g_h=40.0f; run(2100);
  CHECK(!g_tone && g_toneStarts==0, "exactly 40% is not below the threshold -> silent");
  g_h=35.0f; run(2100); show();
  CHECK(g_toneStarts>0, "35% (< 40%) -> buzzer sounds");
  int s=g_toneStarts; run(2000);
  CHECK(g_toneStarts>=s+3, "buzzer keeps beeping while dry");
  CHECK(lcd.rows[1]=="Status: DRY     " && !ledOn(), "LCD Status: DRY, LED off (it is bright)");
  g_analog=900; run(300); show();
  CHECK(lcd.rows[1]=="Status: DRY+DARK" && ledOn() && g_tone==g_tone, "dry AND dark -> DRY+DARK, LED on");
  // display modes
  press(); show();
  CHECK(mode==1 && lcd.rows[0]=="Humidity: 35.0% " && lcd.rows[1]=="Min 40%: TOO DRY", "button -> HUMIDITY mode");
  press(); show();
  CHECK(mode==2 && lcd.rows[0].substr(0,9)=="Light:13%" && lcd.rows[1]=="LED ON  - DARK  ", "button -> LIGHT mode (raw value shown)");
  press(); show();
  CHECK(mode==0, "button -> back to OVERVIEW");
  g_in[BUTTON_PIN]=LOW; run(3000); g_in[BUTTON_PIN]=HIGH; run(100);
  CHECK(mode==1, "holding the button changes mode only once");
  // recover
  g_h=55.0f; g_analog=100; run(2200); show();
  CHECK(!g_tone && !ledOn() && lcd.rows[1]=="Min 40%: OK     ", "humidity back to 55% and bright -> buzzer off, LED off");
  // extremes fit on the LCD
  g_analog=0; run(300); press(); show();
  CHECK(lcd.rows[0].size()==16 && lcd.rows[0]=="Light:100% r0   ", "100% light fits in 16 characters");
  g_analog=1023; run(300); show();
  CHECK(lcd.rows[0]=="Light:0% r1023  ", "full dark (raw 1023) fits");
  // DHT failure keeps last value
  g_h=NAN; run(2100); press(); show();
  CHECK(mode==0 && lcd.rows[0]=="H:55% Light:0%  ", "failed DHT read keeps last humidity");
  printf("\n%s (%d failures)\n", fails?"SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails;
}
