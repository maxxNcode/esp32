#pragma once
#include "Arduino.h"
class LiquidCrystal_I2C { public: std::string rows[2]; int r=0,c=0;
 LiquidCrystal_I2C(int,int,int){} void init(){} void backlight(){} void clear(){rows[0]=rows[1]="";}
 void setCursor(int cc,int rr){c=cc;r=rr;}
 void print(const char*s){ std::string&row=rows[r]; if(row.size()<(size_t)c+strlen(s)) row.resize(c+strlen(s),' '); row.replace(c,strlen(s),s); c+=strlen(s);} 
 void print(const String&s){ print(s.c_str()); } };
#include <cstring>
