#!/bin/sh
# Runs the visitor counter logic on a PC with fake hardware (time, PIR, button, LCD, 7-segment).
cd "$(dirname "$0")"
sed 's/#include <Wire.h>/#include "Wire.h"/; s/#include <LiquidCrystal_I2C.h>/#include "LiquidCrystal_I2C.h"/' ../visitor_counter_uno.ino > sketch.cpp
g++ -std=c++17 -I. -o test test.cpp && ./test
