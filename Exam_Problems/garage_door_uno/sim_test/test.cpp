#include <cstring>
#include "Arduino.h"
unsigned long g_now=0; long g_distance=999; int g_pins[64]; bool g_tone=false; int g_toneCount=0; int g_angle=-1;
int g_pwm[64];
Stream Serial;
#include "sketch.cpp"
#define Serial2 bt
int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok:   %s\n",msg);}while(0)
void run(unsigned long ms){ for(unsigned long i=0;i<ms;i++){ g_now++; loop(); } }
bool sent(const char*m){ for(auto&s:Serial2.out) if(s==m) return true; return false; }
int main(){
  setup(); run(1500);
  CHECK(state==CLOSED && g_angle==0 && g_pins[LED_PIN]==LOW, "starts CLOSED, servo 0, LED off");
  CHECK(Serial2.out.size()>0 && sent("DOOR CLOSED") && sent("VEHICLE: NO"), "reports DOOR CLOSED / VEHICLE: NO to phone");
  // OPEN
  Serial2.out.clear(); g_toneCount=0; Serial2.feed("OPEN"); run(300);
  CHECK(state==OPENING && g_pins[LED_PIN]==HIGH, "OPEN (no newline) -> OPENING, LED on");
  CHECK(g_toneCount>0, "buzzer sounds during opening");
  run(2500);
  CHECK(state==OPEN && g_angle==90 && !g_tone, "door fully OPEN at 90, buzzer off");
  CHECK(g_pwm[LED_PIN]==LED_LEVEL && g_pins[BOARD_LED]==HIGH, "LED at safe PWM level + built-in L LED on");
  CHECK(sent("DOOR OPENING") && sent("DOOR OPEN"), "phone got DOOR OPENING and DOOR OPEN");
  CHECK(lcd.rows[0]=="Door: OPEN      ", "LCD line 1 = Door: OPEN");
  // vehicle present, CLOSE refused
  Serial2.out.clear(); g_distance=8; run(300);
  CHECK(sent("VEHICLE: YES"), "phone gets VEHICLE: YES as soon as it changes");
  Serial2.out.clear(); g_toneCount=0; Serial2.feed("CLOSE\n"); run(300);
  CHECK(state==OPEN && g_angle==90, "CLOSE refused while vehicle present (door stays open)");
  CHECK(sent("BLOCKED - VEHICLE DETECTED"), "phone got BLOCKED - VEHICLE DETECTED");
  CHECK(lcd.rows[1].substr(0,16)=="BLOCKED: Car in!", "LCD shows BLOCKED: Car in!");
  run(2000); printf("      LCD: [%s] [%s]\n", lcd.rows[0].c_str(), lcd.rows[1].c_str());
  CHECK(lcd.rows[1]=="Car: YES     8cm", "after 2 s LCD returns to Car: YES 8cm");
  CHECK(g_toneCount==0, "no buzzer when blocked");
  // vehicle gone, CLOSE works
  g_distance=50; run(2500); Serial2.out.clear(); Serial2.feed("CLOSE"); run(3000);
  CHECK(state==CLOSED && g_angle==0 && g_pwm[LED_PIN]==0 && g_pins[BOARD_LED]==LOW, "CLOSE with no vehicle -> CLOSED, LED off");
  CHECK(lcd.rows[1]=="Car: NO     50cm", "LCD line 2 = Car: NO 50cm");
  // vehicle appears while closing -> reopen
  Serial2.feed("OPEN"); run(2500); Serial2.out.clear(); Serial2.feed("CLOSE"); run(500);
  CHECK(state==CLOSING, "closing started");
  g_distance=5; run(300);
  CHECK(state==OPENING || state==OPEN, "vehicle appears while closing -> door opens again");
  CHECK(sent("BLOCKED - VEHICLE DETECTED"), "phone told BLOCKED");
  run(2500); CHECK(state==OPEN && g_angle==90, "door back fully open");
  // CLOSE during opening turns buzzer off
  g_distance=50; Serial2.feed("CLOSE"); run(2500); Serial2.feed("OPEN"); run(400); Serial2.feed("CLOSE"); run(250);
  CHECK(state==CLOSING && !g_tone, "CLOSE while opening -> closing and buzzer silent");
  run(3000); CHECK(state==CLOSED && !g_tone, "ends CLOSED, buzzer off");
  // Serial monitor commands also work, lowercase too
  Serial.feed("open\n"); run(2500); CHECK(state==OPEN, "Serial Monitor 'open' works");
  printf("\n%s (%d failures)\n", fails?"SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails;
}
