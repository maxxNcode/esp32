#pragma once
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
typedef unsigned char byte;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#ifndef NAN
#define NAN (0.0f/0.0f)
#endif
using std::isnan;
extern unsigned long g_ms; extern int g_in[64]; extern int g_out[64]; extern int g_pwm[64]; extern bool g_tone; extern int g_toneStarts;
inline unsigned long millis(){ return g_ms; }
inline void delay(unsigned long ms){ g_ms+=ms; }
inline void pinMode(int,int){}
inline int digitalRead(int p){ return g_in[p]; }
extern int g_analog;
inline int analogRead(int){ return g_analog; }
#define A0 14
inline long map(long x,long a,long b,long c,long d){ return (x-a)*(d-c)/(b-a)+c; }
template<class T> T constrain(T x,T a,T b){ return x<a?a:(x>b?b:x); }
inline void digitalWrite(int p,int v){ g_out[p]=v; }
inline void analogWrite(int p,int v){ g_pwm[p]=v; }
inline void tone(int,unsigned){ if(!g_tone) g_toneStarts++; g_tone=true; }
inline void noTone(int){ g_tone=false; }
class String : public std::string { public:
  String(){} String(const char*s):std::string(s){} String(const std::string&s):std::string(s){}
  String(int v):std::string(std::to_string(v)){}
  String(float v,int d){ char b[32]; snprintf(b,sizeof b,"%.*f",d,v); assign(b); }
  String substring(size_t a,size_t b) const { return String(std::string::substr(a,b-a)); }
  String operator+(const String&o) const { return String(std::string(*this)+std::string(o)); }
  String operator+(const char*o) const { return String(std::string(*this)+o); }
  String& operator+=(const char*o){ append(o); return *this; }
};
inline String operator+(const char*a,const String&b){ return String(std::string(a)+std::string(b)); }
struct SerialT { void begin(long){} template<class T> void print(T){} template<class T> void print(T,int){} template<class T> void println(T){} };
extern SerialT Serial;
