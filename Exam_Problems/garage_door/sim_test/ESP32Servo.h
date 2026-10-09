#pragma once
extern int g_angle;
class Servo { public: void setPeriodHertz(int){} void attach(int,int,int){} void write(int a){ g_angle=a; } };
