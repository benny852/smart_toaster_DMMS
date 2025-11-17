#include "ModeUI.h"
#include "DisplayUI.h"
#include "Input.h"
#include "sensorManager.h"

namespace
{
    int selectionIndex = 0;
    int yesOrNoIndex = 0;
}

namespace ModeUI
{

    void begin()
    {
        selectionIndex = 0;
        yesOrNoIndex = 0;
        Input::resetEncoder();
        DisplayUI::showModeSelection(selectionIndex);
    }

    bool mainMenuStep(Mode &selectedMode)
    {
        int delta = Input::getEncoderDelta();
        if (delta != 0)
        {
            selectionIndex += (delta > 0 ? 1 : -1);
            if (selectionIndex < 0)
                selectionIndex = 2;
            if (selectionIndex > 2)
                selectionIndex = 0;
            DisplayUI::showModeSelection(selectionIndex);
        }

        Input::updateButton();
        if (!Input::consumeButtonPress())
        {
            return false;
        }

        switch (selectionIndex)
        {
        case 0:
            selectedMode = Mode::Toast;
            break;
        case 1:
            selectedMode = Mode::Sensors;
            break;
        case 2:
            selectedMode = Mode::Logo;
            break;
        default:
            selectedMode = Mode::Toast;
            break;
        }

        return true;
    }

    bool runYesNoDialog()
    {
        yesOrNoIndex = 0;
        Input::resetEncoder();
        DisplayUI::showYesNo(yesOrNoIndex, F("Is the Temperature Jig Setup?"));

        for (;;)
        {
            int delta = Input::getEncoderDelta();
            if (delta != 0)
            {
                yesOrNoIndex += (delta > 0 ? 1 : -1);
                if (yesOrNoIndex < 0)
                    yesOrNoIndex = 1;
                if (yesOrNoIndex > 1)
                    yesOrNoIndex = 0;
                DisplayUI::showYesNo(yesOrNoIndex, F("Is the Temperature Jig Setup?"));
            }

            Input::updateButton();
            if (Input::consumeButtonPress())
            {
                while (Input::isButtonDown())
                {
                    delay(10);
                }
                return (yesOrNoIndex == 0); // true = temp-guided, false = weight-only
            }

            delay(5);
        }
    }

    void runToastingFlow(bool tempGuided)
    {
        // --- Capture initial bread temperature for loss adjustment ---
        sensorsUpdate();
        SensorSnapshot initialSnap = getSensorSnapshot();
        float breadStartTempC = initialSnap.tempC;

        // Map temp to loss fraction: <=12.5C -> 15% loss, >=20C -> 10% loss, linear in between
        const float coldTempC = 12.5f;
        const float warmTempC = 20.0f;
        const float coldLossFrac = 0.15f;
        const float warmLossFrac = 0.10f;
        float lossFrac = warmLossFrac;
        if (breadStartTempC <= coldTempC)
        {
            lossFrac = coldLossFrac;
        }
        else if (breadStartTempC >= warmTempC)
        {
            lossFrac = warmLossFrac;
        }
        else
        {
            float t = (breadStartTempC - coldTempC) / (warmTempC - coldTempC); // 0..1
            lossFrac = coldLossFrac - t * (coldLossFrac - warmLossFrac);
        }
        bool showFrozen = breadStartTempC < warmTempC;
        float extraPercent = (lossFrac - warmLossFrac) * 100.0f;
        if (extraPercent < 0.0f)
            extraPercent = 0.0f;

        // --- Determine starting weight (detect bread placement) ---
        float baseline = initialSnap.weightG;
        float startWeight = baseline;
        const float addedThresholdG = 5.0f;     // change to detect bread
        const unsigned long waitForBreadMs = 15000; // max wait 15s
        unsigned long waitStart = millis();

        DisplayUI::showPlaceBread(showFrozen, extraPercent);

        for (;;)
        {
            sensorsUpdate();
            SensorSnapshot s = getSensorSnapshot();
            float w = s.weightG;
            if (fabsf(w - baseline) > addedThresholdG)
            {
                startWeight = w;
                break;
            }
            if (millis() - waitStart > waitForBreadMs)
            {
                startWeight = w; // fallback to whatever is on the scale
                break;
            }
            delay(50);
        }

        // --- Show calibrating screen until weight stabilizes ---
        const float stableDeltaG = 0.5f;
        const unsigned long stableWindowMs = 1000;
        unsigned long stableStart = millis();
        float lastW = startWeight;
        for (;;)
        {
            sensorsUpdate();
            SensorSnapshot s = getSensorSnapshot();
            float w = s.weightG;
            DisplayUI::showCalibrating(w - baseline);

            if (fabsf(w - lastW) < stableDeltaG)
            {
                if (millis() - stableStart > stableWindowMs)
                {
                    startWeight = w;
                    break;
                }
            }
            else
            {
                stableStart = millis();
            }
            lastW = w;
            delay(100);
        }

        // --- Toast until target weight loss achieved ---
        const float targetWeight = startWeight * (1.0f - lossFrac);
        const float requiredLoss = startWeight - targetWeight;
        const unsigned long maxToastMs = 5UL * 60UL * 1000UL; // 5-minute fail-safe
        unsigned long startMs = millis();

        // --- Optional temperature stabilization ---
        bool tempStable = false;
        unsigned long tempStableStart = 0;
        const float tempThresholdC = 120.0f;       // reach at least ~120C
        const float tempDeltaStable = 0.5f;        // change within 0.5C
        const unsigned long tempStableWindow = 20000; // 20 seconds within band
        const unsigned long postStableHoldMs = 150000; // 2.5 minutes after stable
        float lastTemp = 0.0f;

        for (;;)
        {
            sensorsUpdate();
            SensorSnapshot s = getSensorSnapshot();
            float w = s.weightG;
            float t = s.tempC;

            // Track temperature stabilization if enabled
            if (tempGuided)
            {
                if (t >= tempThresholdC && fabsf(t - lastTemp) < tempDeltaStable)
                {
                    if (!tempStable)
                    {
                        tempStable = true;
                        tempStableStart = millis();
                    }
                }
                else
                {
                    tempStable = false;
                    tempStableStart = 0;
                }
                lastTemp = t;
            }

            float weightProgress = 0.0f;
            if (requiredLoss > 0.01f)
            {
                weightProgress = (startWeight - w) / requiredLoss;
            }
            if (weightProgress < 0.0f)
                weightProgress = 0.0f;
            if (weightProgress > 1.0f)
                weightProgress = 1.0f;

            float timeProgress = 0.0f;
            if (tempGuided && tempStable && tempStableStart > 0)
            {
                unsigned long sinceStable = millis() - tempStableStart;
                timeProgress = (float)sinceStable / (float)postStableHoldMs;
                if (timeProgress > 1.0f)
                    timeProgress = 1.0f;
            }

            float progress = tempGuided ? max(weightProgress, timeProgress) : weightProgress;

            DisplayUI::showToastingProcess(progress);

            unsigned long elapsed = millis() - startMs;
            bool weightDone = (w <= targetWeight);
            bool tempHoldDone = tempGuided && tempStable && tempStableStart > 0 && (millis() - tempStableStart >= postStableHoldMs);
            if (weightDone || tempHoldDone || elapsed >= maxToastMs)
            {
                break;
            }

            delay(100);
        }

        DisplayUI::showToastReady();

        // Hold on the "Toast Ready" screen until user clicks to exit
        for (;;)
        {
            Input::updateButton();
            if (Input::consumeButtonPress())
            {
                while (Input::isButtonDown())
                {
                    delay(2);
                }
                break;
            }
            delay(5);
        }
    }

} // namespace ModeUI
