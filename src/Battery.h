#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

class Battery {
public:
    Battery(int pin, float vRef, float r1, float r2);
    void begin();
    void update();
    float getVoltage();
    int getBatteryLevel();
    bool isCharging();
    bool isLowBattery();

private:
    int _pin;
    float _vRef;
    float _r1;
    float _r2;
    float _voltage;
    int _battery_level;
    bool _is_charging;
    bool _is_low_battery;
    float _last_voltage;
    uint32_t _last_update_time;
};

#endif // BATTERY_H
