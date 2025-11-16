#pragma once

// The intention of this class is to create the Tca_Selection_Channel Function to optimise the mains readability

#include <Wire.h>

#define TCA_ADDR 0x70

// int8_t ? 8-bit signed integer | holding values from -128 to 127 | in our case, hardware channel level data
// uint8_t ? 8-bit unsigned integer | holding values from 0 to 255

static inline void tcaSelectChannel(int8_t ch)
{
    if (ch < 0)
        return;

    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << (ch & 7));
    Wire.endTransmission();
}