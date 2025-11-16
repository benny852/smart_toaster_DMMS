#include "ModeUI.h"
#include "DisplayUI.h"
#include "Input.h"

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

        while (Input::isButtonDown())
        {
            delay(10);
        }

        return true;
    }

    bool runYesNoDialog()
    {
        yesOrNoIndex = 0;
        DisplayUI::showYesNo(yesOrNoIndex, F("Start toasting?"));

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
                DisplayUI::showYesNo(yesOrNoIndex, F("Start toasting?"));
            }

            Input::updateButton();
            if (Input::consumeButtonPress())
            {
                while (Input::isButtonDown())
                {
                    delay(10);
                }
                return (yesOrNoIndex == 1);
            }

            delay(10);
        }
    }

    void runToastingFlow()
    {
        const unsigned long toastTimeMs = 5000; // placeholder: 5s

        unsigned long start = millis();
        for (;;)
        {
            unsigned long elapsed = millis() - start;
            float progress = (float)elapsed / (float)toastTimeMs;
            if (progress > 1.0f)
                progress = 1.0f;

            DisplayUI::showToastingProcess(progress);

            if (elapsed >= toastTimeMs)
            {
                break;
            }
            delay(50);
        }

        DisplayUI::showToastReady();
    }

} // namespace ModeUI
