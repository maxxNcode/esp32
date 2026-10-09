#pragma once
extern int g_angle;
class Servo { public: void attach(int){} void write(int a){ g_angle=a; } };
