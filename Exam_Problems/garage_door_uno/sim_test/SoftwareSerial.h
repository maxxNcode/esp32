#pragma once
#include "Arduino.h"
class SoftwareSerial : public Stream { public: SoftwareSerial(int,int){} };
extern SoftwareSerial *g_bt;
