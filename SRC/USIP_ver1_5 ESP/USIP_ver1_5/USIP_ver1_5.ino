/*
// PROGRAM FOR USIP_1  (DISCOVERY III)
// USIP Copyright 2025 NASA, University of Bridgeport Extreme Environments Laboratory Bridgeport, CT.
// All rights reserved.
//
// SOFTWARE VERSION NUMBER: see Config.h (USIP_VERSION)
//
// Version history (truncated; see git log for full timeline):
//   Ver 0.x .. 1.2  2017-2025  Initial implementation, telemetry tuning,
//                              Mega + HC-05 ↔ HC-05 + Mega.
//   Ver 1.3, 3/22/25  Reconstruction for ESP-32 data comm, Serial 3 disabled.
//   Ver 1.4, 3/23/25  Removed Serial 2 for monkey, transmits over SerialBT.
//   Ver 1.5, 4/29/26  Aggressive restructure: split into modules, char buffers
//                     replace Arduino String, cooperative scheduler, explicit
//                     enum-based state machine, FLIGHT/GROUND build modes,
//                     STUB_PEER bench-test mode, BT auto-reconnect, abort
//                     debounce ring buffer, GPS checksum bounds checks.
//                     See ../../../.claude/plans/ for the refactor plan.
//
*/

#include <Arduino.h>
#include "Config.h"
#include "State.h"
#include "Scheduler.h"
#include "Sensors.h"
#include "BTLink.h"
#include "GPS.h"
#include "Phase.h"
#include "Time.h"
#include "Commands.h"
#include "Telemetry.h"

// ----------------------------------------------------------------------------
// setup()
// ----------------------------------------------------------------------------

void setup() {
  // Pins
  pinMode(PIN_ABORT, OUTPUT);
  digitalWrite(PIN_ABORT, HIGH);    // de-energize cutdown relay (LOW-active)
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_MKXMT, OUTPUT);
  pinMode(PIN_MKRCV, INPUT);

  // LED startup blink (10x at 100 ms each)
  for (int i = 0; i < 10; ++i) {
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
  }

  // Console + radio UARTs.
  // NOTE: Serial1 default RX/TX on ESP-32 are GPIO 9/10 — bonded to internal
  // SPI flash on -WROOM modules. If your board is -WROOM, prefer:
  //   Serial1.begin(BAUD_RADIO, SERIAL_8N1, RX_PIN, TX_PIN);
  // with safe pins. See Config.h note 1.
  Serial.begin(BAUD_CONSOLE);
  Serial1.begin(BAUD_RADIO);

  Serial.println();
  Serial.print("USIP Version ");
  Serial.println(USIP_VERSION);
  Serial.print("BUILD_MODE = ");
  Serial.println(BUILD_MODE == FLIGHT ? "FLIGHT" : "GROUND");
#if STUB_PEER
  Serial.println("STUB_PEER = 1 (synthetic monkey replies, no real BT)");
#endif

  // Module init
  State_init();
  Sensors_init();
  Time_init();
  BT_init();   // blocks up to ~10 s on each connect attempt; see Config.h note 4

  // Scheduler tasks. Cadences from Config.h.
  Scheduler_register(TICK_1HZ_MS,                  Sensors_tick1Hz);
  Scheduler_register(TICK_PHASE_MS,                Phase_tick10s);
  Scheduler_register(TICK_TELEMETRY_MS,            Telemetry_tick60s);
  Scheduler_register(MONKEY_POLL_OK_SEC * 1000U,   BT_pollMonkeyOk);
  Scheduler_register(MONKEY_POLL_DOWN_SEC * 1000U, BT_pollMonkeyDown);
  Scheduler_register(BT_RECONNECT_TICK_MS,         BT_reconnectTick);

  Serial.println("USIP setup complete.");
}

// ----------------------------------------------------------------------------
// loop()
// ----------------------------------------------------------------------------
//
// Per-iteration polls (these don't have a cadence — they drain whatever is
// available from the underlying transport):
//   * BT_pollRx          — drain SerialBT (or STUB_PEER synth queue)
//   * GPS_pollRx         — frame NMEA on Serial1 RX
//   * Commands_pollConsole — frame operator commands on Serial RX
//   * Time_tick          — advance mission clock at 1 Hz; cheap when nothing to do
//   * Phase_pinTick      — manage the cutdown pulse window
//   * Telemetry_drainBursts — flush any pending APRS metadata bursts
//   * Scheduler_tick     — run any scheduled tasks whose interval elapsed
//
// The scheduler-driven tasks (1 Hz sensors, 10 s phase, 30 s monkey poll,
// 60 s telemetry, 5 s BT reconnect) all live inside Scheduler_tick().

void loop() {
  BT_pollRx();
  GPS_pollRx();
  Commands_pollConsole();
  Time_tick();
  Phase_pinTick();
  Telemetry_drainBursts();
  Scheduler_tick();
}
