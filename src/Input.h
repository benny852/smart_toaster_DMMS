#pragma once
#include <Arduino.h>

// Rotary encoder pins
constexpr int ENC_PIN_A = 35;
constexpr int ENC_PIN_B = 34;
constexpr int ENC_PIN_SW = 32;

namespace Input
{
    void begin();
    int getEncoderDelta();
    void updateButton();
    bool consumeButtonPress();
    bool isButtonDown();
} // namespace Input
