#include "load_cell.h"

static bool eepromReady = false;
static inline void eepromBeginIfNeeded()
{
#if defined(ESP32)
    if (!eepromReady)
    {
        EEPROM.begin(64);
        eepromReady = true;
    }
#endif
}

bool LoadCellNAU7802::begin(float countsPerGram)
{
    if (!_scale.begin())
        return false;

    // Your original ritual
    _scale.setGain(128);
    _scale.setSampleRate(10);
    _scale.calibrateAFE();

#if LC_EEPROM_EN
    if (isnan(countsPerGram))
    {
        loadFromEEPROM();
    }
    else
    {
        _cpg = countsPerGram;
    }
#else
    if (!isnan(countsPerGram))
        _cpg = countsPerGram;
#endif

    _scale.setCalibrationFactor(_cpg);

    // quick tare: average a few readings for zero offset
    _zeroOffset = averagedReading(16);
    _scale.setZeroOffset(_zeroOffset);

    // init stream
    _w_raw_g = 0;
    _w_g = 0;
    _baseline_g = 0;
    _inited = true;
    return true;
}

void LoadCellNAU7802::update()
{
    if (!_inited)
        return;
    uint32_t now = millis();
    if (now - _last < _period)
        return;
    _last = now;

    // SparkFun helper: getWeight(true) does avail/waiting internally
    float g = _scale.getWeight(true);

    // deadband small/negative like your sketch
    if (fabsf(g) <= LC_DEADBAND_G)
        g = 0.0f;

    _w_raw_g = g;

    // EMA smoothing so prints are stable
    _w_g = (1.0f - LC_SMOOTH_ALPHA) * _w_g + LC_SMOOTH_ALPHA * g;

    // ---- Optional OLED feedback ----
    if (_oled)
    {
        _oled->clearDisplay();
        _oled->setTextSize(1);
        _oled->setTextColor(SSD1306_WHITE);
        _oled->setCursor(0, 0);
        _oled->printf("Weight: %.1fg\n", _w_g);
        _oled->printf("Delta: %.1fg\n", deltaFromBaseline_g());
        _oled->printf("CPG: %.1f", _cpg);
        _oled->display();
    }
}

void LoadCellNAU7802::tare()
{
    _zeroOffset = averagedReading(32);
    _scale.setZeroOffset(_zeroOffset);
    _baseline_g = 0.0f;
#if LC_EEPROM_EN
    saveToEEPROM(); // keep zero across boots if you like
#endif
}

bool LoadCellNAU7802::calibrate(float knownMass_g)
{
    if (knownMass_g <= 0.0f)
        return false;

    // ensure zeroed first
    tare();

    // ask user to place known mass, settle a moment (caller can also wait)
    delay(300);

    // average counts at this load
    long countsWithMass = averagedReading(32);
    long deltaCounts = countsWithMass - _zeroOffset;
    if (deltaCounts <= 0)
        return false;

    _cpg = (float)deltaCounts / knownMass_g;
    _scale.setCalibrationFactor(_cpg);

#if LC_EEPROM_EN
    saveToEEPROM();
#endif
    return true;
}

void LoadCellNAU7802::setCountsPerGram(float cpg)
{
    if (cpg > 0.0f)
    {
        _cpg = cpg;
        _scale.setCalibrationFactor(_cpg);
#if LC_EEPROM_EN
        saveToEEPROM();
#endif
    }
}

long LoadCellNAU7802::averagedReading(int samples)
{
    long sum = 0;
    int n = max(1, samples);
    for (int i = 0; i < n;)
    {
        if (_scale.available())
        {
            sum += _scale.getReading();
            ++i;
        }
    }
    return sum / n;
}

void LoadCellNAU7802::handleSerial()
{
    if (!Serial.available())
        return;
    char c = (char)Serial.read();

    if (c == 't')
    {
        Serial.println("[LC] Tare...");
        tare();
        Serial.println("[LC] Tare done.");
    }
    else if (c == 'c')
    {
        Serial.printf("[LC] Calibrate using %.1fg... remove all weight, press any key\n", LC_KNOWN_MASS_G);
        while (!Serial.available())
            delay(10);
        while (Serial.available())
            Serial.read();

        tare();

        Serial.println("[LC] Place known mass, press any key when stable...");
        while (!Serial.available())
            delay(10);
        while (Serial.available())
            Serial.read();

        if (calibrate(LC_KNOWN_MASS_G))
        {
            Serial.printf("[LC] New countsPerGram = %.3f\n", _cpg);
        }
        else
        {
            Serial.println("[LC] Calibration failed (deltaCounts <= 0?)");
        }
    }
    else if (c == 'b')
    {
        setBaselineToCurrent();
        Serial.printf("[LC] Baseline set to %.2fg\n", _baseline_g);
    }
}

void LoadCellNAU7802::loadFromEEPROM()
{
#if LC_EEPROM_EN
    eepromBeginIfNeeded();
    if (EEPROM.read(LC_EE_BASE) != LC_EE_SIG)
        return;

    uint8_t *p = (uint8_t *)&_cpg;
    for (int i = 0; i < 4; i++)
        p[i] = EEPROM.read(LC_EE_BASE + 1 + i);

    long z = 0;
    for (int i = 0; i < 4; i++)
        ((uint8_t *)&z)[i] = EEPROM.read(LC_EE_BASE + 5 + i);
    _zeroOffset = z;
#endif
}

void LoadCellNAU7802::saveToEEPROM()
{
#if LC_EEPROM_EN
    eepromBeginIfNeeded();
    EEPROM.write(LC_EE_BASE, LC_EE_SIG);
    uint8_t *p = (uint8_t *)&_cpg;
    for (int i = 0; i < 4; i++)
        EEPROM.write(LC_EE_BASE + 1 + i, p[i]);

    uint8_t *q = (uint8_t *)&_zeroOffset;
    for (int i = 0; i < 4; i++)
        EEPROM.write(LC_EE_BASE + 5 + i, q[i]);

#if defined(ESP32)
    EEPROM.commit();
#endif
#endif
}
