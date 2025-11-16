// TemperatureSensor.h
#pragma once
#include <Adafruit_MLX90614.h>
#include "tca_breakout.h"

class TemperatureSensor
{
public:
    explicit TemperatureSensor(int8_t muxCh, uint32_t periodMs = 100)
        : ch(muxCh), period(periodMs) {}
    bool begin()
    {
        tcaSelectChannel(ch);
        return mlx.begin();
    }
    void update(uint32_t now)
    {
        if (now - last < period)
            return;
        last = now;
        tcaSelectChannel(ch);
        ambC = mlx.readAmbientTempC();
        objC = mlx.readObjectTempC();
    }
    double ambient() const { return ambC; }
    double object() const { return objC; }

private:
    int8_t ch;
    uint32_t period;
    uint32_t last = 0;
    Adafruit_MLX90614 mlx;
    double ambC = 0, objC = 0;
};
