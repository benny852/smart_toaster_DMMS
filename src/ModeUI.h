#pragma once
#include <Arduino.h>

enum class Mode : uint8_t
{
    Toast = 0,
    Sensors = 1,
    Logo = 2
};

namespace ModeUI
{
    void begin();
    bool mainMenuStep(Mode &selectedMode);
    bool runYesNoDialog();
    void runToastingFlow();
} // namespace ModeUI
