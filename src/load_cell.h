#pragma once
#include <Arduino.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h> // NAU7802
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_SSD1306.h> // for optional OLED display

// =============== User knobs ===============
#define LC_EEPROM_EN 1         // set 0 to disable EEPROM save/restore
#define LC_KNOWN_MASS_G 200.0f // grams used during quick calibration
#define LC_DEADBAND_G 0.5f     // treat |weight| < this as zero
#define LC_SMOOTH_ALPHA 0.2f   // EMA smoothing 0..1 (higher = snappier)
#define LC_PRINT_INTERVAL 250  // ms between prints (example main)
// =========================================

// EEPROM layout (ESP32 needs EEPROM.begin(size) once)
static constexpr int LC_EE_BASE = 0x30;
static constexpr uint8_t LC_EE_SIG = 0x5A;

class LoadCellNAU7802
{
public:
    LoadCellNAU7802(uint32_t periodMs = 200)
        : _period(periodMs) {}

    // Call once in setup() after Wire.begin()
    // If you already have a saved calibration, pass NAN to load it from EEPROM.
    // Otherwise pass your known factor (counts per gram).
    bool begin(float countsPerGram = NAN);

    // Call often (e.g., every loop); it self-throttles by _period
    void update();

    // Commands
    void tare();                       // fast tare (zero offset)
    bool calibrate(float knownMass_g); // place known mass, computes new cal factor
    void setCountsPerGram(float cpg);
    float countsPerGram() const { return _cpg; }

    // Getters (smoothed)
    float weight_g() const { return _w_g; }  // net (after zero offset), smoothed
    float raw_g() const { return _w_raw_g; } // unsmoothed latest
    float baseline_g() const { return _baseline_g; }
    void setBaselineToCurrent() { _baseline_g = _w_g; }
    float deltaFromBaseline_g() const { return _w_g - _baseline_g; }

    // Optional: handle simple serial commands ('t' tare, 'c' calibrate, 'b' baseline)
    void handleSerial();

    void attachDisplay(Adafruit_SSD1306 *disp) { _oled = disp; }

private:
    // helpers
    long averagedReading(int samples = 16);
    void loadFromEEPROM();
    void saveToEEPROM();
    Adafruit_SSD1306 *_oled = nullptr;

    NAU7802 _scale;
    uint32_t _period, _last = 0;

    // calibration + zeroing
    float _cpg = 700.0f; // counts per gram (example default, overwritten)
    long _zeroOffset = 0;

    // data
    float _w_raw_g = 0.0f;    // latest net grams (unsmoothed)
    float _w_g = 0.0f;        // smoothed grams, deadband applied
    float _baseline_g = 0.0f; // for “delta” readout

    bool _inited = false;
};
