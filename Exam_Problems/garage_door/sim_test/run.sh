#!/bin/sh
# Runs the garage door logic on a PC with fake hardware (time, sensor, Bluetooth, servo, LCD).
cd "$(dirname "$0")"
sed 's/#include <ESP32Servo.h>/#include "ESP32Servo.h"/; s/#include <Wire.h>/#include "Wire.h"/; s/#include <LiquidCrystal_I2C.h>/#include "LiquidCrystal_I2C.h"/' ../garage_door.ino > sketch.cpp
g++ -std=c++17 -I. -o test test.cpp && ./test
