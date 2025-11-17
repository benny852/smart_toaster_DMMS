#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- OLED config ----------
constexpr int SCREEN_WIDTH  = 128;
constexpr int SCREEN_HEIGHT = 64;

constexpr int OLED_SDA_PIN  = 21;      // ESP32 SDA
constexpr int OLED_SCL_PIN  = 22;      // ESP32 SCL
constexpr int OLED_ADDR     = 0x3C;    // 0x3C or 0x3D
constexpr int OLED_RESET    = -1;      // -1 if reset not on a GPIO
// ---------------------------------

namespace DisplayUI {

  bool begin();
  Adafruit_SSD1306 &getDisplay();

  void showInitError(const __FlashStringHelper *msg = F("Display init failed"));

  void showSplash();  // logo on boot

  // Main mode selection: 0 = Toast, 1 = Sensors, 2 = Logo
  void showModeSelection(int selectionIndex);

  // Live sensor screen
  void showSensorShowcase(float tempC,
                          float weightGrams,
                          uint8_t r8,
                          uint8_t g8,
                          uint8_t b8,
                          float brightness);

  // Toasting progress: 0..1
  void showToastingProcess(float progress01);

  // Toast done screen
  void showToastReady();

  // Place bread prompt; optionally show frozen info and extra percent
  void showPlaceBread(bool showFrozen, float extraPercent);

  // Calibrating added weight (shows current grams)
  void showCalibrating(float weightG);

  // Yes/No dialog
  void showYesNo(int yesOrNoIndex,
                 const __FlashStringHelper *question = F("Start toasting?"));

} // namespace DisplayUI
