#include "Input.h"

namespace
{
    volatile int encoderPos = 0;
    volatile int lastA = 0;
    int lastReported = 0;

    bool buttonPressed = false;
    unsigned long lastDebounce = 0;
    bool lastButtonState = HIGH;
}

void IRAM_ATTR handleEncoder()
{
    int a = digitalRead(ENC_PIN_A);
    int b = digitalRead(ENC_PIN_B);

    if (a != lastA)
    {
        if (a == b)
        {
            encoderPos++;
        }
        else
        {
            encoderPos--;
        }
        lastA = a;
    }
}

namespace Input
{

    void begin()
    {
        // Match the original tested wiring: plain inputs on A/B, pull-up on button
        pinMode(ENC_PIN_A, INPUT);
        pinMode(ENC_PIN_B, INPUT);
        pinMode(ENC_PIN_SW, INPUT_PULLUP);

        lastA = digitalRead(ENC_PIN_A);

        attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), handleEncoder, CHANGE);
        attachInterrupt(digitalPinToInterrupt(ENC_PIN_B), handleEncoder, CHANGE);
    }

    int getEncoderDelta()
    {
        int current = encoderPos;
        int delta = current - lastReported;
        lastReported = current;
        return delta;
    }

    void resetEncoder()
    {
        lastReported = encoderPos;
    }

    void updateButton()
    {
        const unsigned long debounceMs = 20;
        bool reading = digitalRead(ENC_PIN_SW);
        unsigned long now = millis();

        if (reading != lastButtonState && (now - lastDebounce) > debounceMs)
        {
            lastDebounce = now;
            lastButtonState = reading;
            if (reading == LOW)
            {
                buttonPressed = true; // only on falling edge
            }
        }
    }

    bool consumeButtonPress()
    {
        if (buttonPressed)
        {
            buttonPressed = false;
            return true;
        }
        return false;
    }

    bool isButtonDown()
    {
        return digitalRead(ENC_PIN_SW) == LOW;
    }

} // namespace Input
