#pragma once
#include <string>
#include <deque>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cctype>
typedef unsigned char byte;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
extern unsigned long g_us; extern int g_in[64]; extern int g_out[64]; extern int g_beeps; extern int g_maxOn;
inline unsigned long millis(){ return g_us/1000; }
inline unsigned long micros(){ return g_us; }
inline void pinMode(int,int){}
inline int digitalRead(int p){ return g_in[p]; }
void digitalWrite(int p,int v);
inline void tone(int,unsigned,unsigned long){ g_beeps++; }
class String : public std::string { public:
  String(){} String(const char*s):std::string(s){} String(const std::string&s):std::string(s){}
  String(long v):std::string(std::to_string(v)){} String(int v):std::string(std::to_string(v)){} String(unsigned v):std::string(std::to_string(v)){}
  String substring(size_t a,size_t b) const { return String(std::string::substr(a,b-a)); }
  String operator+(const String&o) const { return String(std::string(*this)+std::string(o)); }
  String operator+(const char*o) const { return String(std::string(*this)+o); }
  String& operator+=(const String&o){ append(o); return *this; }
  String& operator+=(const char*o){ append(o); return *this; }
  bool operator!=(const char*o) const { return std::string(*this)!=o; }
};
inline String operator+(const char*a,const String&b){ return String(std::string(a)+std::string(b)); }
struct SerialT { void begin(long){} template<class T> void print(T){} template<class T> void println(T){} };
extern SerialT Serial;
