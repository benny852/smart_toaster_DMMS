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
  delay(3000);

  ModeUI::begin();
}

void loop()
{
  Mode selectedMode;

  if (!ModeUI::mainMenuStep(selectedMode))
  {
    // Stay in menu; avoid extra work to keep rotary snappy
    return;
  }

  switch (selectedMode)
  {
  case Mode::Toast:
  {
    // Clear any residual press from the menu selection
    Input::updateButton();
    Input::consumeButtonPress();
    while (Input::isButtonDown())
    {
      delay(2);
      Input::updateButton();
      Input::consumeButtonPress();
    }

    bool tempGuided = ModeUI::runYesNoDialog(); // true if jig setup (temp-assisted)
    ModeUI::runToastingFlow(tempGuided);
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

      delay(10);
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
      delay(5);
    }
    break;
  }
  }

  while (Input::isButtonDown())
  {
    delay(2);
  }

  ModeUI::begin();
}
