#pragma once
#include <Arduino.h>
#include <Adafruit_TCS34725.h>

// Index your three sensors (SDA1, SDA3, SDA4)
enum ColourSensorIdx : uint8_t
{
    CS_0 = 0,
    CS_1 = 1,
    CS_2 = 2,
    CS_COUNT = 3
};

struct ColourReading
{
    uint16_t r16, g16, b16, c16; // raw channels
    uint8_t r8, g8, b8;          // normalized 0..255
    uint8_t r_out, g_out, b_out; // LED values written
    float brightness;            // 0..1 from C/C_ref
    uint16_t cref;               // calibration used
};

// Call in setup()
void colourSetup();

// Calibrate white for one sensor (place white target, steady light)
bool colourCalibrateWhite(ColourSensorIdx idx, uint16_t samples = 16);

// Update ONE sensor (selects TCA channel internally) and update LEDs
void convertColourToRGB(ColourSensorIdx idx);

// Update ALL sensors (convenience)
void colourTickAll();

// Read back the latest computed values
bool colourGetReading(ColourSensorIdx idx, ColourReading &out);

// Optional: tweak per-sensor white reference
void colourSetCref(ColourSensorIdx idx, uint16_t cref);
uint16_t colourGetCref(ColourSensorIdx idx);
