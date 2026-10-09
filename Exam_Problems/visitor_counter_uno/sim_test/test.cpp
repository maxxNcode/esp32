#include "Arduino.h"
unsigned long g_us=0; int g_in[64]; int g_out[64]; int g_beeps=0; int g_maxOn=0;
SerialT Serial;
#include "sketch.cpp"
void digitalWrite(int p,int v){ g_out[p]=v; int on=0; for(int i=0;i<7;i++) on+=g_out[SEG_PINS[i]]; if(on>g_maxOn) g_maxOn=on; }
int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok:   %s\n",msg);}while(0)
// run for ms milliseconds, loop() every 100 us; record which segments were ever lit
int litMask=0;
void run(unsigned long ms){ unsigned long end=g_us+ms*1000; litMask=0; while(g_us<end){ g_us+=100; loop(); for(int i=0;i<7;i++) if(g_out[SEG_PINS[i]]) litMask|=1<<i; } }
int main(){
  g_in[BUTTON_PIN]=HIGH; g_in[PIR_PIN]=LOW;
  setup();
  run(5000);
  CHECK(lcd.rows[1].find("warm-up")!=std::string::npos, "LCD shows PIR warm-up countdown");
  g_in[PIR_PIN]=HIGH; run(1000); g_in[PIR_PIN]=LOW;
  CHECK(visitors==0, "motion during warm-up is ignored");
  run(11000);
  CHECK(lcd.rows[1]=="Ready           ", "after warm-up LCD says Ready");
  CHECK(lcd.rows[0]=="Visitors: 00    ", "LCD shows Visitors: 00");
  CHECK(litMask==DIGITS[0], "7-segment shows 0");
  CHECK(g_maxOn<=1, "never more than ONE segment on at a time (low current, no resistors)");
  // visitor 1
  g_in[PIR_PIN]=HIGH; run(300);
  CHECK(visitors==1 && g_beeps==1, "motion starts -> count 1 + one beep");
  CHECK(lcd.rows[0]=="Visitors: 01    ", "LCD Visitors: 01");
  // continuous motion: stays HIGH 10 s, with a short 300 ms dropout in between
  run(4000); g_in[PIR_PIN]=LOW; run(300); g_in[PIR_PIN]=HIGH; run(6000);
  CHECK(visitors==1 && g_beeps==1, "one long continuous motion (even with a short dropout) counts only once");
  CHECK(litMask==DIGITS[1], "7-segment shows 1");
  // motion ends, quiet > 1 s, new motion -> visitor 2
  g_in[PIR_PIN]=LOW; run(1500); g_in[PIR_PIN]=HIGH; run(200);
  CHECK(visitors==2 && g_beeps==2, "new motion after quiet time -> count 2 + beep");
  // 9 more visitors -> 11
  for(int i=0;i<9;i++){ g_in[PIR_PIN]=LOW; run(1200); g_in[PIR_PIN]=HIGH; run(500); }
  g_in[PIR_PIN]=LOW; run(1700);
  CHECK(visitors==11, "11 separate visitors counted");
  CHECK(lcd.rows[0]=="Visitors: 11    ", "LCD Visitors: 11");
  CHECK(litMask==DIGITS[1], "7-segment shows last digit (1)");
  CHECK(lcd.rows[1]=="Ready           ", "message returns to Ready");
  // reset button with bounce
  g_in[BUTTON_PIN]=LOW; run(5); g_in[BUTTON_PIN]=HIGH; run(5); g_in[BUTTON_PIN]=LOW; run(300); g_in[BUTTON_PIN]=HIGH; run(300);
  CHECK(visitors==0, "button resets counter to zero");
  CHECK(lcd.rows[0]=="Visitors: 00    " && lcd.rows[1]=="Counter reset   ", "LCD Visitors: 00 + Counter reset");
  run(20);
  CHECK(litMask==DIGITS[0], "7-segment back to 0");
  CHECK(g_beeps==11, "no extra beeps from reset");
  // holding the button does not keep resetting / counting
  g_in[PIR_PIN]=HIGH; run(300); g_in[BUTTON_PIN]=LOW; run(2000);
  CHECK(visitors==0, "visitor then button held -> reset once (count 0)");
  g_in[BUTTON_PIN]=HIGH; g_in[PIR_PIN]=LOW; run(1500); g_in[PIR_PIN]=HIGH; run(300);
  CHECK(visitors==1, "counting works again after reset");
  printf("\n%s (%d failures)\n", fails?"SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails;
}
