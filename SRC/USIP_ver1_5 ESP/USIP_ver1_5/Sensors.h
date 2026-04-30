// Sensors.h
// BME280 (I2C) + analog sensors. The 1 Hz tick reads BME280 and, in FLIGHT
// build mode only, reads the analog ADC pins for OTemp / ITemp1 / Batv1.
// In GROUND mode the sensor cache stays at the defaults set in State_init().

#ifndef USIP_SENSORS_H
#define USIP_SENSORS_H

// Initialize BME280 with 0x77 → 0x76 fallback. Returns true on success.
// Stores result in g_state.bme_ok.
bool Sensors_init();

// 1 Hz tick. Updates g_state.ps_temp / .ps_pressure / .pressure_alt from
// BME280 (if BME_OK). Under FLIGHT mode also samples the analog pins.
void Sensors_tick1Hz();

#endif  // USIP_SENSORS_H
