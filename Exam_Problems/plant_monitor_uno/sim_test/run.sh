#!/bin/sh
# Runs the plant monitor logic on a PC with a fake DHT11, LDR, button, LED, buzzer and LCD.
cd "$(dirname "$0")"
sed 's/#include <DHT.h>/#include "DHT.h"/; s/#include <Wire.h>/#include "Wire.h"/; s/#include <LiquidCrystal_I2C.h>/#include "LiquidCrystal_I2C.h"/' ../plant_monitor_uno.ino > sketch.cpp
g++ -std=c++17 -I. -o test test.cpp && ./test
