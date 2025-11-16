#include "DisplayUI.h"
#include <Wire.h>

namespace
{
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

namespace DisplayUI
{

    bool begin()
    {
        Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
        Wire.setClock(100000);

        if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
        {
            return false;
        }

        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setTextWrap(true);
        display.display();
        return true;
    }

    Adafruit_SSD1306 &getDisplay()
    {
        return display;
    }

    void showInitError(const __FlashStringHelper *msg)
    {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println(msg);
        display.display();
    }

    void showSplash()
    {
        display.clearDisplay();

        // Simple placeholder splash – you can replace with your bitmap
        display.drawRect(20, 12, SCREEN_WIDTH - 40, 32, SSD1306_WHITE);
        display.setCursor(30, 22);
        display.setTextSize(1);
        display.println(F("Smart Toaster"));
        display.setCursor(34, 36);
        display.println(F("OLED Menu"));

        display.display();
    }

    void showModeSelection(int selectionIndex)
    {
        display.clearDisplay();

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(F("Select Mode"));

        const int yIcon = 16;
        const int xToast = 16;
        const int xSensor = 64;
        const int xLogo = 112;

        // Simple placeholder "icons"
        display.fillRect(xToast - 8, yIcon, 16, 12, SSD1306_WHITE);
        display.drawRect(xSensor - 8, yIcon, 16, 12, SSD1306_WHITE);
        display.drawCircle(xLogo, yIcon + 6, 6, SSD1306_WHITE);

        display.setCursor(xToast - 12, 32);
        display.println(F("Toast"));

        display.setCursor(xSensor - 16, 32);
        display.println(F("Sensors"));

        display.setCursor(xLogo - 12, 32);
        display.println(F("Logo"));

        int underlineX = xToast;
        switch (selectionIndex)
        {
        case 0:
            underlineX = xToast;
            break;
        case 1:
            underlineX = xSensor;
            break;
        case 2:
            underlineX = xLogo;
            break;
        }

        display.drawLine(underlineX - 14, 44, underlineX + 14, 44, SSD1306_WHITE);
        display.display();
    }

    void showSensorShowcase(float tempC,
                            float weightGrams,
                            uint8_t r8,
                            uint8_t g8,
                            uint8_t b8,
                            float brightness)
    {
        display.clearDisplay();

        // Frame
        display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        display.drawLine(0, 12, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.drawLine(SCREEN_WIDTH / 2, 12, SCREEN_WIDTH / 2, SCREEN_HEIGHT, SSD1306_WHITE);
        display.drawLine(0, 32, SCREEN_WIDTH, 32, SSD1306_WHITE);

        display.setTextSize(1);
        display.setTextWrap(false);
        display.setCursor(2, 2);
        display.println(F("Sensor Showcase"));

        // Top-right: RGB
        display.setCursor(SCREEN_WIDTH / 2 + 2, 16);
        display.print(F("R:"));
        display.print(r8);
        display.setCursor(SCREEN_WIDTH / 2 + 2, 24);
        display.print(F("G:"));
        display.print(g8);
        display.setCursor(SCREEN_WIDTH / 2 + 2, 32);
        display.print(F("B:"));
        display.print(b8);

        // Top-left: temperature
        display.setCursor(4, 18);
        display.print(F("Temp: "));
        display.print(tempC, 1);
        display.println(F("C"));

        // Bottom-left: weight
        display.setCursor(4, 38);
        display.print(F("Weight: "));
        display.print(weightGrams / 1000.0f, 2);
        display.println(F(" kg"));

        // Bottom-right: brightness
        display.setCursor(SCREEN_WIDTH / 2 + 2, 46);
        display.print(F("Bright: "));
        display.print(brightness * 100.0f, 1);
        display.println(F("%"));

        display.display();
    }

    void showToastingProcess(float progress01)
    {
        if (progress01 < 0.0f)
            progress01 = 0.0f;
        if (progress01 > 1.0f)
            progress01 = 1.0f;

        display.clearDisplay();

        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println(F("Toasting..."));

        const int barX = 10;
        const int barY = 24;
        const int barW = SCREEN_WIDTH - 20;
        const int barH = 12;

        display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
        int fillW = static_cast<int>(barW * progress01);
        if (fillW > 2)
        {
            display.fillRect(barX + 1, barY + 1, fillW - 2, barH - 2, SSD1306_WHITE);
        }

        int percent = static_cast<int>(progress01 * 100.0f + 0.5f);
        display.setCursor(barX, barY + barH + 10);
        display.print(percent);
        display.println(F("%"));

        display.display();
    }

    void showToastReady()
    {
        display.clearDisplay();

        display.setTextSize(2);
        display.setCursor(10, 20);
        display.println(F("Toast"));
        display.setCursor(10, 40);
        display.println(F("Ready!"));

        display.display();
    }

    void showYesNo(int yesOrNoIndex,
                   const __FlashStringHelper *question)
    {
        display.clearDisplay();

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(question);

        display.setCursor(20, 32);
        display.print(F("NO"));

        display.setCursor(80, 32);
        display.print(F("YES"));

        int x = (yesOrNoIndex == 0) ? 20 : 80;
        int w = (yesOrNoIndex == 0) ? 18 : 24;
        display.drawLine(x, 40, x + w, 40, SSD1306_WHITE);

        display.display();
    }

} // namespace DisplayUI
