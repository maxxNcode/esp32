#pragma once
#define DHT11 11
#define DHT22 22
extern float g_h, g_t;
class DHT { public: DHT(int,int){} void begin(){} float readHumidity(){ return g_h; } float readTemperature(){ return g_t; } };
