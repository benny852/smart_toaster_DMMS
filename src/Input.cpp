#include "Input.h"

namespace
{
    volatile int encoderPos = 0;
    volatile int lastA = 0;
    int lastReported = 0;

    bool buttonPressed = false;
    unsigned long lastDebounce = 0;
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

    void updateButton()
    {
        const unsigned long debounceMs = 250;

        if (digitalRead(ENC_PIN_SW) == LOW)
        {
            unsigned long now = millis();
            if (now - lastDebounce > debounceMs)
            {
                buttonPressed = true;
                lastDebounce = now;
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
