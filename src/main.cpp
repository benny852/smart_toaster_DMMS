#include <Arduino.h>
#include "DisplayUI.h"
#include "Input.h"
#include "ModeUI.h"
#include "sensorManager.h"

void setup()
{
  Serial.begin(115200);

  if (!DisplayUI::begin())
  {
    DisplayUI::showInitError(F("SSD1306 alloc failed"));
    while (true)
    {
      delay(1000);
    }
  }

  Input::begin();
  sensorsBegin();

  DisplayUI::showSplash();
  delay(2000);

  ModeUI::begin();
}

void loop()
{
  Mode selectedMode;

  if (!ModeUI::mainMenuStep(selectedMode))
  {
    // Still navigating menu
    sensorsUpdate(); // keep sensors fresh even in menu
    return;
  }

  switch (selectedMode)
  {
  case Mode::Toast:
  {
    bool doToast = ModeUI::runYesNoDialog();
    if (doToast)
    {
      ModeUI::runToastingFlow();
    }
    break;
  }

  case Mode::Sensors:
  {
    while (true)
    {
      sensorsUpdate();
      SensorSnapshot s = getSensorSnapshot();

      DisplayUI::showSensorShowcase(
          s.tempC,
          s.weightG,
          s.r8, s.g8, s.b8,
          s.brightness);

      Input::updateButton();
      if (Input::consumeButtonPress())
      {
        break;
      }

      delay(100);
    }
    break;
  }

  case Mode::Logo:
  {
    DisplayUI::showSplash();
    while (true)
    {
      Input::updateButton();
      if (Input::consumeButtonPress())
      {
        break;
      }
      delay(20);
    }
    break;
  }
  }

  while (Input::isButtonDown())
  {
    delay(10);
  }

  ModeUI::begin();
}
