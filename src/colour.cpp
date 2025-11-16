#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_TCS34725.h>

#include "colour.h"

// ---------- TCA helper ----------
#include "tca_breakout.h" // must define: inline void tcaSelect(int8_t ch);

// ---------- One shared TCS object (we switch channels via TCA) ----------
static Adafruit_TCS34725 TCS_breakout(
    TCS34725_INTEGRATIONTIME_614MS,
    TCS34725_GAIN_1X);

// ---------- Your three TCA channels (SDA1, SDA3, SDA4) ----------
static constexpr int8_t COLOR_CH[CS_COUNT] = {
    1, // CS_0 -> TCA_SDA1
    3, // CS_1 -> TCA_SDA3
    4  // CS_2 -> TCA_SDA4
};

// ---------- LEDC channels + brightness ----------
static constexpr int CH_R = 0;
static constexpr int CH_G = 1;
static constexpr int CH_B = 2;

static constexpr float BRIGHTNESS = 0.8f; // master scaling 0..1

// Attach your pins somewhere in your board init:
// ledcAttachPin(RED_PIN, CH_R); etc.

// ---------- Per-sensor white reference ----------
static uint16_t C_ref[CS_COUNT] = {1200, 1200, 1200};

// ---------- Optional EEPROM persistence (ESP32 needs EEPROM.begin) ----------
static constexpr int EEPROM_BASE = 0x00;
static constexpr uint8_t EEPROM_SIG = 0xA5;
static bool eeprom_ready = false;

static inline void eepromBeginIfNeeded()
{
#if defined(ESP32)
    if (!eeprom_ready)
    {
        EEPROM.begin(32);
        eeprom_ready = true;
    }
#endif
}

static inline void eepromLoadCrefs()
{
    eepromBeginIfNeeded();
    if (EEPROM.read(EEPROM_BASE) != EEPROM_SIG)
        return;
    for (int i = 0; i < CS_COUNT; i++)
    {
        int off = EEPROM_BASE + 1 + i * 2;
        uint16_t v = (uint16_t)EEPROM.read(off) | ((uint16_t)EEPROM.read(off + 1) << 8);
        if (v >= 10 && v < 65535)
            C_ref[i] = v;
    }
}

static inline void eepromSaveCrefs()
{
    eepromBeginIfNeeded();
    EEPROM.write(EEPROM_BASE, EEPROM_SIG);
    for (int i = 0; i < CS_COUNT; i++)
    {
        int off = EEPROM_BASE + 1 + i * 2;
        EEPROM.write(off, (uint8_t)(C_ref[i] & 0xFF));
        EEPROM.write(off + 1, (uint8_t)(C_ref[i] >> 8));
    }
#if defined(ESP32)
    EEPROM.commit();
#endif
}

// ---------- Last-read storage ----------
static ColourReading lastReading[CS_COUNT] = {};

// ---------- Small helpers ----------
static inline uint8_t clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static inline void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    ledcWrite(CH_R, r);
    ledcWrite(CH_G, g);
    ledcWrite(CH_B, b);
}

// ================= PUBLIC API =================

void colourSetCref(ColourSensorIdx idx, uint16_t cref)
{
    if (idx < CS_COUNT && cref >= 10)
        C_ref[idx] = cref;
}

uint16_t colourGetCref(ColourSensorIdx idx)
{
    return (idx < CS_COUNT) ? C_ref[idx] : 0;
}

void colourSetup()
{
    // LEDC setup (attach pins elsewhere)
    ledcSetup(CH_R, 5000, 8);
    ledcSetup(CH_G, 5000, 8);
    ledcSetup(CH_B, 5000, 8);
    setRGB(0, 0, 0);

    // Load saved C_refs if present
    eepromLoadCrefs();

    // Probe each sensor on its TCA channel
    bool ok_all = true;
    for (int i = 0; i < CS_COUNT; i++)
    {
        tcaSelectChannel(COLOR_CH[i]);
        bool ok = TCS_breakout.begin();
        Serial.printf("TCS@CH%d: %s\n", COLOR_CH[i], ok ? "FOUND" : "NOT FOUND");
        ok_all &= ok;
    }
    if (!ok_all)
        Serial.println("One or more TCS34725 sensors not found.");
}

bool colourCalibrateWhite(ColourSensorIdx idx, uint16_t samples)
{
    if (idx >= CS_COUNT)
        return false;

    tcaSelectChannel(COLOR_CH[idx]);
    if (!TCS_breakout.begin())
        return false; // ensure device up on this channel

    delay(10);

    uint32_t sumC = 0;
    uint16_t r, g, b, c;
    const uint16_t n = samples ? samples : 8;

    for (uint16_t i = 0; i < n; i++)
    {
        TCS_breakout.getRawData(&r, &g, &b, &c);
        sumC += c;
        delay(5);
    }

    uint16_t avg = (uint16_t)(sumC / n);
    if (avg < 10)
        avg = 10;
    C_ref[idx] = avg;

    eepromSaveCrefs();

    Serial.printf("Calibrated C_ref[%u] = %u (CH%d)\n",
                  (unsigned)idx, avg, COLOR_CH[idx]);
    return true;
}

void convertColourToRGB(ColourSensorIdx idx)
{
    if (idx >= CS_COUNT)
        return;

    tcaSelectChannel(COLOR_CH[idx]);

    uint16_t r16, g16, b16, c16;
    TCS_breakout.getRawData(&r16, &g16, &b16, &c16);

    // Prepare snapshot
    ColourReading r{};
    r.r16 = r16;
    r.g16 = g16;
    r.b16 = b16;
    r.c16 = c16;
    r.cref = C_ref[idx];

    if (c16 < 5)
    {
        setRGB(0, 0, 0);
        r.r8 = r.g8 = r.b8 = 0;
        r.r_out = r.g_out = r.b_out = 0;
        r.brightness = 0.0f;
        lastReading[idx] = r;
        return;
    }

    // Normalize by clear channel (preserve hue)
    uint32_t rn = (uint32_t)r16 * 255u / c16;
    uint32_t gn = (uint32_t)g16 * 255u / c16;
    uint32_t bn = (uint32_t)b16 * 255u / c16;

    r.r8 = rn > 255u ? 255u : (uint8_t)rn;
    r.g8 = gn > 255u ? 255u : (uint8_t)gn;
    r.b8 = bn > 255u ? 255u : (uint8_t)bn;

    float brightness = (float)c16 / (float)C_ref[idx];
    if (brightness > 1.0f)
        brightness = 1.0f;
    if (brightness < 0.0f)
        brightness = 0.0f;
    r.brightness = brightness;

    r.r_out = (uint8_t)((float)r.r8 * brightness * BRIGHTNESS + 0.5f);
    r.g_out = (uint8_t)((float)r.g8 * brightness * BRIGHTNESS + 0.5f);
    r.b_out = (uint8_t)((float)r.b8 * brightness * BRIGHTNESS + 0.5f);

    setRGB(r.r_out, r.g_out, r.b_out);

    lastReading[idx] = r;

    // Optional debug (comment out if noisy)
    static uint32_t last = 0;
    if (millis() - last > 300)
    {
        last = millis();
        Serial.printf("[CS%u CH%d] RGBC=%u,%u,%u,%u | norm=%3u,%3u,%3u | Cref=%u br=%.2f | LED=%3u,%3u,%3u\n",
                      (unsigned)idx, COLOR_CH[idx],
                      r16, g16, b16, c16, r.r8, r.g8, r.b8, r.cref, r.brightness,
                      r.r_out, r.g_out, r.b_out);
    }
}

void colourTickAll()
{
    convertColourToRGB(CS_0);
    convertColourToRGB(CS_1);
    convertColourToRGB(CS_2);
}

bool colourGetReading(ColourSensorIdx idx, ColourReading &out)
{
    if (idx >= CS_COUNT)
        return false;
    out = lastReading[idx];
    return true;
}
