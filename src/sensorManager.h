#pragma once
#include <Arduino.h>

#include "temperature.h"
#include "load_cell.h"
#include "colour.h"

struct SensorSnapshot
{
    float tempC;
    float weightG;
    uint8_t r8, g8, b8;
    float brightness;
};

void sensorsBegin();
void sensorsUpdate();
SensorSnapshot getSensorSnapshot();
