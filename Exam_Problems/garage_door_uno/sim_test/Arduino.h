#pragma once
#include <string>
#include <deque>
#include <cstdio>
#include <cstdint>
#include <cctype>
#include <vector>
typedef unsigned char byte;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define SERIAL_8N1 0
extern unsigned long g_now; extern long g_distance; extern int g_pins[64]; extern bool g_tone; extern int g_toneCount;
inline unsigned long millis(){ return g_now; }
inline void delay(unsigned long ms){ g_now += ms; }
inline void delayMicroseconds(unsigned){}
inline void pinMode(int,int){}
inline void digitalWrite(int p,int v){ g_pins[p]=v; }
inline long pulseIn(int,int,unsigned long){ return g_distance>=999?0:(long)(g_distance*2/0.034)+1; }
inline void tone(int,unsigned){ g_tone=true; g_toneCount++; }
inline void noTone(int){ g_tone=false; }
class String : public std::string { public:
  String(){} String(const char*s):std::string(s){} String(const std::string&s):std::string(s){}
  String(long v):std::string(std::to_string(v)){} String(int v):std::string(std::to_string(v)){}
  void trim(){ while(!empty()&&isspace(back())) pop_back(); while(!empty()&&isspace(front())) erase(0,1);} 
  void toUpperCase(){ for(auto&c:*this) c=toupper(c);} 
  String substring(size_t a,size_t b) const { return String(std::string::substr(a,b-a)); }
  String operator+(const String&o) const { return String(std::string(*this)+std::string(o)); }
  String operator+(const char*o) const { return String(std::string(*this)+o); }
  String& operator+=(char c){ push_back(c); return *this; }
  String& operator+=(const String&o){ append(o); return *this; }
};
inline String operator+(const char*a,const String&b){ return String(std::string(a)+std::string(b)); }
class Stream { public: std::deque<char> in; std::vector<std::string> out; std::string cur;
  int available(){ return in.size(); } int read(){ char c=in.front(); in.pop_front(); return c; }
  void println(const String&s){ out.push_back(s); } void println(const char*s){ out.push_back(s); }
  void begin(long){} void begin(long,int,int,int){} void feed(const char*s){ for(;*s;s++) in.push_back(*s);} };
typedef Stream HardwareSerial;
extern Stream Serial, Serial2;
extern int g_pwm[64];
inline void analogWrite(int p,int v){ g_pwm[p]=v; g_pins[p]= v>0; }
