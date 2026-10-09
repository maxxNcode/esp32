#!/bin/sh
# Runs the Uno garage door logic on a PC with fake hardware (time, sensor, Bluetooth, servo, LCD).
cd "$(dirname "$0")"
sed 's/#include <Servo.h>/#include "Servo.h"/; s/#include <SoftwareSerial.h>/#include "SoftwareSerial.h"/; s/#include <Wire.h>/#include "Wire.h"/; s/#include <LiquidCrystal_I2C.h>/#include "LiquidCrystal_I2C.h"/' ../garage_door_uno.ino > sketch.cpp
g++ -std=c++17 -I. -o test test.cpp && ./test
