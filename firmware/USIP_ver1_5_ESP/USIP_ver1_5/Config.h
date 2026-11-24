// Config.h
// USIP_ver1_5 ground-station configuration constants.
//
// Copyright 2025 NASA, University of Bridgeport, Discovery Museum Bridgeport, CT.
// All rights reserved.
//
// =============================================================================
// ESP-32 deployment notes (verify these on first power-up — they are the
// most likely things to bite the operator):
//
//   1. Serial1 default pins on ESP-32 are GPIO 9/10, which on -WROOM modules
//      are bonded to internal SPI flash. The current Serial1.begin(BAUD_RADIO)
//      with no pin args may fail silently or corrupt flash. Prefer the
//      explicit form: Serial1.begin(BAUD_RADIO, SERIAL_8N1, RX, TX). See
//      PIN_RADIO_RX / PIN_RADIO_TX below.
//   2. GPIO 13 is usually NOT the LED on ESP-32 dev boards. Common LED pins
//      are GPIO 2 (Wemos), GPIO 5 (HUZZAH32), or none. The blink in setup()
//      may silently do nothing on this board. Confirm and update PIN_LED.
//   3. BME280 default I2C address is 0x77, but many breakouts ship at 0x76.
//      bme.begin() with no arg only tries 0x77. Add a 0x76 fallback in
//      Sensors_init() (Phase 5).
//   4. ESP-32 task watchdog (TWDT) is enabled with a 5 s timeout on the loop
//      task. SerialBT.connect() can block 2-10 s. Either feed the WDT
//      around the connect call, or accept periodic resets.
//   5. BluetoothSerial is Bluetooth Classic (RFCOMM/SPP), NOT BLE. HC-05 is
//      classic. Do not "modernize" to BLEDevice.h — the link will break.
//   6. SerialBT.println() emits "\r\n". Verified safe: airborne Monkey_v2
//      caps inStr2 at 11 chars and parses substring(0, 9), so the trailing
//      "\r\n" is excluded from the equality check.
// =============================================================================

#ifndef USIP_CONFIG_H
#define USIP_CONFIG_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// Software version
// -----------------------------------------------------------------------------
#define USIP_VERSION "1.5"

// -----------------------------------------------------------------------------
// Build mode (FLIGHT vs GROUND)
//
// Edit BUILD_MODE below to switch between flight and ground builds.
// FLIGHT  = full sensor suite, real BT to airborne HC-05.
// GROUND  = bench mode; analog sensors (OTemp, ITemp1, Batv1) are not present
//           on the ground bench, so their telemetry channels publish sentinel
//           values. Packet shape stays identical so the upstream APRS decoder
//           does not need a mode toggle.
//
// The constexpr below is intentionally a runtime `if` rather than #ifdef so
// the unused branch is still type-checked at compile time. -Os dead-code-
// eliminates the unused branch — no binary cost vs #ifdef.
// -----------------------------------------------------------------------------
enum BuildMode : uint8_t { GROUND = 0, FLIGHT = 1 };
constexpr BuildMode BUILD_MODE = GROUND;

// -----------------------------------------------------------------------------
// Stub peer mode (bench testing without HC-05/Mega)
//
// When set to 1, BTLink.cpp synthesizes fake monkey replies instead of
// driving real BT. Lets us verify the parser path, telemetry, and abort
// logic on an ESP-32 alone. Wired in Phase 5b. Always leave 0 for flight.
// -----------------------------------------------------------------------------
#define STUB_PEER 1

// -----------------------------------------------------------------------------
// Pin assignments (ESP-32 GPIO numbers)
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_LED      = 13;  // status LED — verify on actual board (see note 2)
constexpr uint8_t PIN_ABORT    = 2;   // balloon cutdown relay, LOW-active
constexpr uint8_t PIN_MKXMT    = 16;  // monkey transmit ready (currently unused)
constexpr uint8_t PIN_MKRCV    = 17;  // monkey receive ready (currently unused)
// Analog sensor pins (FLIGHT mode only — wired in Phase 7):
//   PIN_OUTSIDE_TEMP    A0   outside temperature
//   PIN_INSIDE_TEMP     A1   inside temperature
//   PIN_TX_TEMP         A2   transmitter temperature
//   PIN_BATTERY_VOLTAGE A3   battery voltage divider

// Serial1 (radio) pin mapping — set explicitly to avoid the GPIO 9/10 flash
// conflict on -WROOM modules. Defaults below are unused; pick safe pins on
// first ESP-32 wiring. Phase 5/7 will wire these into Serial1.begin().
//   PIN_RADIO_RX
//   PIN_RADIO_TX

// -----------------------------------------------------------------------------
// HC-05 Bluetooth pairing
//
// BT_PEER_MAC_INIT is a brace-enclosed initializer (not a constexpr array),
// because BluetoothSerial::connect() takes a non-const uint8_t[] and won't
// accept a const/constexpr array directly. The .ino instantiates a local
// uint8_t[6] from this macro.
// -----------------------------------------------------------------------------
#define BT_PEER_MAC_INIT             { 0x00, 0x14, 0x03, 0x05, 0x01, 0x62 }
#define BT_PIN_CSTR                  "1234"
#define BT_MASTER_NAME               "ESP32test"
constexpr uint8_t  BT_CONNECT_ATTEMPTS       = 5;
constexpr uint32_t BT_CONNECT_RETRY_DELAY_MS = 2000;

// -----------------------------------------------------------------------------
// Serial baud rates
// -----------------------------------------------------------------------------
constexpr uint32_t BAUD_CONSOLE = 19200;  // IDE serial monitor
constexpr uint32_t BAUD_RADIO   = 19200;  // TinyTrak4 APRS radio
// BAUD_BT not configurable — BluetoothSerial uses RFCOMM framing, not UART.

// -----------------------------------------------------------------------------
// Atmosphere / altitude
// -----------------------------------------------------------------------------
constexpr float SLP_HPA      = 1013.25f;  // sea-level pressure (hPa) for altimetry
constexpr float ALT_M_TO_FT  = 3.281f;    // meters to feet conversion

// -----------------------------------------------------------------------------
// Flight envelope
// -----------------------------------------------------------------------------
constexpr float    MAX_ALT_DEFAULT       = 99.0f;  // thousands of feet; 99 disables abort
constexpr uint32_t ENDABORT_PULSE_MS     = 4000;   // cutdown relay pulse width
constexpr uint8_t  ABORT_DEBOUNCE_COUNT  = 12;     // consecutive over-threshold samples
constexpr float    ALT_BUMP_WARN_FT      = 50.0f;  // altitude jump for debug warning

// -----------------------------------------------------------------------------
// Timing / cadence
// -----------------------------------------------------------------------------
constexpr uint32_t TICK_1HZ_MS              = 1000;    // sensor read tick
constexpr uint32_t TICK_PHASE_MS             = 10000;  // flight-phase machine tick
constexpr uint32_t TICK_TELEMETRY_MS        = 60000;   // APRS telemetry packet
constexpr uint32_t BT_RECONNECT_TICK_MS     = 5000;    // BT reconnect attempt cadence
constexpr uint32_t MONKEY_REPLY_TIMEOUT_MS  = 65000;   // demote MonkeyOK if no reply
constexpr uint32_t GPS_TIMEOUT_MS           = 150000;  // demote GPS_valid (2.5 min)
constexpr uint8_t  MONKEY_POLL_OK_SEC       = 30;      // "MONKEY 12" ping cadence (sec)
constexpr uint8_t  MONKEY_POLL_DOWN_SEC     = 20;      // "MONKEY 09" ping cadence (sec)
constexpr uint8_t  APRS_BURST_MINUTE_MOD    = 20;      // every 20 minutes
constexpr uint8_t  APRS_BURST_MINUTE        = 5;       // ...at minute 5/25/45

#endif  // USIP_CONFIG_H
