#include "sensorManager.h"
#include "tca_breakout.h"

// Adjust to your actual mux channel for MLX90614
static constexpr int8_t TEMP_MUX_CH = 0;

// Update periods ms (match your original design if needed)
static constexpr uint32_t TEMP_PERIOD_MS = 100;
static constexpr uint32_t LC_PERIOD_MS = 200;

// Types assumed from your library; tweak if names differ
static TemperatureSensor g_tempSensor(TEMP_MUX_CH, TEMP_PERIOD_MS);
static LoadCellNAU7802 g_loadCell(LC_PERIOD_MS);

void sensorsBegin()
{
    if (!g_tempSensor.begin())
    {
        Serial.println(F("[TEMP] MLX90614 init failed"));
    }

    if (!g_loadCell.begin(NAN))
    {
        Serial.println(F("[LC] NAU7802 init failed"));
    }

    colourSetup();
    // Optional white calibration on CS_0:
    // colourCalibrateWhite(CS_0, 16);
}

void sensorsUpdate()
{
    uint32_t now = millis();
    g_tempSensor.update(now);
    g_loadCell.update();
    colourTickAll();
}

SensorSnapshot getSensorSnapshot()
{
    SensorSnapshot s{};
    s.tempC = static_cast<float>(g_tempSensor.object());
    s.weightG = g_loadCell.weight_g();

    ColourReading cr{};
    if (colourGetReading(CS_0, cr))
    {
        s.r8 = cr.r8;
        s.g8 = cr.g8;
        s.b8 = cr.b8;
        s.brightness = cr.brightness;
    }
    else
    {
        s.r8 = s.g8 = s.b8 = 0;
        s.brightness = 0.0f;
    }

    return s;
}
