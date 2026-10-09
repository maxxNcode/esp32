#include "Arduino.h"
unsigned long g_ms=0; int g_in[64]; int g_out[64]; int g_pwm[64]; bool g_tone=false; int g_toneStarts=0;
float g_h=55.0f, g_t=28.4f;
SerialT Serial;
#include "sketch.cpp"
int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok:   %s\n",msg);}while(0)
void run(unsigned long ms){ for(unsigned long i=0;i<ms;i++){ g_ms++; loop(); } }
void show(){ printf("      LCD: [%s] [%s]\n", lcd.rows[0].c_str(), lcd.rows[1].c_str()); }
bool ledOn(){ return g_pwm[LED_PIN]>0 && g_out[BOARD_LED]==HIGH; }
void press(){ g_in[BUTTON_PIN]=LOW; run(3); g_in[BUTTON_PIN]=HIGH; run(3); g_in[BUTTON_PIN]=LOW; run(200); g_in[BUTTON_PIN]=HIGH; run(200); }
int main(){
  g_in[BUTTON_PIN]=HIGH;
  setup(); run(100); show();
  CHECK(lcd.rows[0]=="Humidity: 55%   " && lcd.rows[1]=="Status: NORMAL  ", "55% -> LCD Humidity: 55% / Status: NORMAL");
  CHECK(!ledOn() && !g_tone, "55%: LED off, buzzer off");
  CHECK(g_pwm[LED_PIN]==0, "LED pin fully off");
  g_h=70.0f; run(2100); show();
  CHECK(!ledOn() && !g_tone && lcd.rows[1]=="Status: NORMAL  ", "exactly 70% is NOT above 70 -> still NORMAL");
  g_h=71.0f; run(2100); show();
  CHECK(ledOn() && g_pwm[LED_PIN]==LED_LEVEL, "71%: warning LED ON (safe PWM level)");
  CHECK(!g_tone && g_toneStarts==0, "71%: buzzer still off");
  CHECK(lcd.rows[1]=="Status: WARNING ", "LCD Status: WARNING");
  g_h=80.0f; run(2100);
  CHECK(ledOn() && g_toneStarts==0, "exactly 80%: LED on, buzzer off");
  g_h=85.0f; run(2100); show();
  CHECK(ledOn() && g_toneStarts>0, "85%: LED ON and buzzer sounding");
  int s1=g_toneStarts; run(2000);
  CHECK(g_toneStarts>s1+3, "buzzer keeps beeping while above 80%");
  CHECK(lcd.rows[1]=="Status: ALARM   ", "LCD Status: ALARM");
  // detailed mode
  press(); show();
  CHECK(detailedMode && lcd.rows[0]=="H:85.0% T:28.4C " && lcd.rows[1]=="ALARM LED+BUZZER", "button -> DETAILED mode (humidity + temperature + outputs)");
  g_h=60.0f; run(2100); show();
  CHECK(!ledOn() && !g_tone && lcd.rows[1]=="NORMAL   all OFF", "back to 60%: LED and buzzer off, detailed status NORMAL");
  CHECK(lcd.rows[0]=="H:60.0% T:28.4C ", "detailed reading updates continuously");
  g_h=75.5f; run(2100); show();
  CHECK(lcd.rows[1]=="WARNING  LED ON ", "detailed WARNING line") ;
  press(); show();
  CHECK(!detailedMode && lcd.rows[0]=="Humidity: 76%   ", "button again -> NORMAL mode (75.5 rounds to 76)");
  // holding the button does not keep toggling
  g_in[BUTTON_PIN]=LOW; run(3000); g_in[BUTTON_PIN]=HIGH; run(100);
  CHECK(detailedMode, "holding the button toggles only once");
  // sensor failure keeps last good value
  g_h=NAN; run(2100); show();
  CHECK(lcd.rows[0]=="H:75.5% T:28.4C ", "failed read keeps the last good value");
  g_h=50.0f; g_t=NAN; run(2100);
  CHECK(lcd.rows[0]=="H:75.5% T:28.4C ", "failed temperature read also ignored");
  g_t=27.0f; run(2100); show();
  CHECK(lcd.rows[0]=="H:50.0% T:27.0C " && !ledOn(), "recovers on next good reading");
  printf("\n%s (%d failures)\n", fails?"SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails;
}
