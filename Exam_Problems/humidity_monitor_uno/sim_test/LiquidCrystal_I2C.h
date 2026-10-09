#pragma once
#include "Arduino.h"
class LiquidCrystal_I2C { public: std::string rows[2]; int r=0,c=0;
 LiquidCrystal_I2C(int,int,int){} void init(){} void backlight(){}
 void setCursor(int cc,int rr){c=cc;r=rr;}
 void print(const char*s){ std::string&row=rows[r]; size_t n=strlen(s); if(row.size()<(size_t)c+n) row.resize(c+n,' '); row.replace(c,n,s); c+=n;}
 void print(const String&s){ print(s.c_str()); } };
