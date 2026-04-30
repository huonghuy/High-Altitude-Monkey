// Sensors.cpp
#include "Sensors.h"
#include "Config.h"
#include "State.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Arduino.h>

namespace {
  Adafruit_BME280 g_bme;
  // SLP can be re-calibrated by the phase machine when prelaunch is detected
  // (currently dead code — the recal is commented out). Keep mutable.
  float g_slp = SLP_HPA;

  // Analog pin assignments (FLIGHT mode only). Unused when BUILD_MODE == GROUND.
  // Verify these against the actual PCB before enabling FLIGHT mode.
  constexpr uint8_t PIN_OUTSIDE_TEMP    = A0;
  constexpr uint8_t PIN_INSIDE_TEMP     = A1;
  constexpr uint8_t PIN_TX_TEMP         = A2;
  constexpr uint8_t PIN_BATTERY_VOLTAGE = A3;

  // ESP-32 ADC is 12-bit by default (0..4095). The original Mega code used
  // 1024 (10-bit). These constants drive the analog → physical conversion.
  constexpr float ADC_FULL_SCALE_COUNTS = 4095.0f;
  constexpr float ADC_VREF_MILLIVOLT    = 3300.0f;     // ESP-32 default Vref
  constexpr float THERMISTOR_SLOPE      = 500.0f;      // mV per degree (legacy)
  constexpr float KELVIN_OFFSET         = 273.0f;
  constexpr float BATT_DIVIDER_GAIN     = 102.85f;     // calibrated 10/31/19 LJR
}

bool Sensors_init() {
  if (g_bme.begin(0x77)) {
    g_state.bme_ok = true;
  } else if (g_bme.begin(0x76)) {
    // Many BME280 breakouts ship at 0x76 instead of 0x77.
    g_state.bme_ok = true;
  } else {
    Serial.println("Could not find a valid BME280 sensor at 0x77 or 0x76 — check wiring/address.");
    Serial.print("SensorID was: 0x");
    Serial.println(g_bme.sensorID(), 16);
    g_state.bme_ok = false;
  }
  return g_state.bme_ok;
}

void Sensors_tick1Hz() {
  if (g_state.bme_ok) {
    g_state.ps_temp      = g_bme.readTemperature();
    g_state.ps_pressure  = g_bme.readPressure();
    g_state.pressure_alt = g_bme.readAltitude(g_slp) * ALT_M_TO_FT;
  }

  if (BUILD_MODE == FLIGHT) {
    // ESP-32 12-bit ADC. The original Mega thermistor formula was
    //   T_C = (raw/1024 * 500) - 273
    // which normalizes raw across 10-bit full scale, multiplies by mV/°C, and
    // subtracts the Kelvin offset. Translated to ESP-32:
    //   T_C = (raw/4095 * 500) - 273
    int o_raw = analogRead(PIN_OUTSIDE_TEMP);
    int i_raw = analogRead(PIN_INSIDE_TEMP);
    int b_raw = analogRead(PIN_BATTERY_VOLTAGE);

    g_state.o_temp  = ((float)o_raw / ADC_FULL_SCALE_COUNTS) * THERMISTOR_SLOPE - KELVIN_OFFSET;
    g_state.i_temp1 = ((float)i_raw / ADC_FULL_SCALE_COUNTS) * THERMISTOR_SLOPE - KELVIN_OFFSET;
    g_state.bat_v1  = (float)b_raw / BATT_DIVIDER_GAIN;
    // PIN_TX_TEMP currently unused — TxTemp comes from MONKEY 13 reply.
    (void)ADC_VREF_MILLIVOLT;  // reserved for future calibrated voltage reads
  }
  // GROUND mode: leave o_temp / i_temp1 / bat_v1 at their State_init defaults.
}
