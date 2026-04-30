// Telemetry.cpp
#include "Telemetry.h"
#include "Config.h"
#include "State.h"
#include <Arduino.h>
#include <stdio.h>

namespace {
  // Telemetry encoding offsets (legacy values from original code at line
  // 1032-1036): each channel is offset so the typical operating range maps
  // to 0..255 after constrain(). Decoder on the operator side knows these.
  constexpr int OFFSET_TEMP   = 100;
  constexpr int OFFSET_VSPEED = 200;
  constexpr int CLAMP_MIN     = 0;
  constexpr int CLAMP_MAX     = 255;

  uint16_t g_tseq    = 0;
  bool     g_parm_pending = false;
  bool     g_unit_pending = false;
  bool     g_equ_pending  = false;
  bool     g_bit_pending  = false;

  // Helper: format the 8-bit status field.
  //   bit 0: GPS_VALID
  //   bit 1: time_sync == TS_LIVE
  //   bit 2: comms_ok
  //   bit 3: alt_cal
  //   bit 4: monkey == MK_OK
  //   bit 5: phase == PRELAUNCH
  //   bit 6: phase == RISING
  //   bit 7: phase == FALLING
  void writeStatusBits(char* dst) {
    dst[0] = (g_state.gps == GPS_VALID)             ? '1' : '0';
    dst[1] = (g_state.time_sync == TS_LIVE)         ? '1' : '0';
    dst[2] = (g_state.comms_ok)                     ? '1' : '0';
    dst[3] = (g_state.alt_cal)                      ? '1' : '0';
    dst[4] = (g_state.monkey == MK_OK)              ? '1' : '0';
    dst[5] = (g_state.phase == PHASE_PRELAUNCH)     ? '1' : '0';
    dst[6] = (g_state.phase == PHASE_RISING)        ? '1' : '0';
    dst[7] = (g_state.phase == PHASE_FALLING)       ? '1' : '0';
  }

  inline int clamp(int v) {
    if (v < CLAMP_MIN) return CLAMP_MIN;
    if (v > CLAMP_MAX) return CLAMP_MAX;
    return v;
  }
}

void Telemetry_tick60s() {
  g_tseq++;

  // Vertical speed: oldest-fix-style estimate from GPS history. Same as
  // original (lines 1031): ((current - t-15) + (t-15 - t-30)/2) / 15.
  float vspeed = ((g_state.gps_alt - g_state.gps_alt1)
                + ((g_state.gps_alt1 - g_state.gps_alt2) / 2.0f)) / 15.0f;

  // FLIGHT vs GROUND: under GROUND, the analog sensors don't exist on the
  // bench. Substitute sentinel 0xFF in the encoded telemetry channels so
  // the operator-side decoder shows "n/a" without us breaking packet
  // shape. (i_temp2, tx_temp, bat_v2 still come from the airborne MONKEY
  // 13 reply in both modes.)
  int telem_TxTemp, telem_ITemp1, telem_ITemp2, telem_OTemp, telem_Vspeed;
  if (BUILD_MODE == FLIGHT) {
    telem_TxTemp = clamp((int)(g_state.tx_temp + OFFSET_TEMP));
    telem_ITemp1 = clamp((int)(g_state.i_temp1 + OFFSET_TEMP));
    telem_ITemp2 = clamp((int)(g_state.i_temp2 + OFFSET_TEMP));
    telem_OTemp  = clamp((int)(g_state.o_temp  + OFFSET_TEMP));
    telem_Vspeed = clamp((int)(vspeed + OFFSET_VSPEED));
  } else {
    telem_TxTemp = clamp((int)(g_state.tx_temp + OFFSET_TEMP));   // from MONKEY 13
    telem_ITemp1 = 0xFF;                                          // sentinel
    telem_ITemp2 = clamp((int)(g_state.i_temp2 + OFFSET_TEMP));   // from MONKEY 13
    telem_OTemp  = 0xFF;                                          // sentinel
    telem_Vspeed = clamp((int)(vspeed + OFFSET_VSPEED));
  }

  char bits[9] = { 0 };
  writeStatusBits(bits);
  bits[8] = 0;

  char packet[64];
  snprintf(packet, sizeof(packet),
           "T#%03u,%d,%d,%d,%d,%d,%s\n",
           (unsigned)g_tseq,
           telem_TxTemp,
           telem_ITemp1,
           telem_ITemp2,
           telem_OTemp,
           telem_Vspeed,
           bits);

  Serial1.print(packet);
  Serial.print(packet);
}

// -- APRS metadata bursts ----------------------------------------------------

void Telemetry_requestParm() { g_parm_pending = true; }
void Telemetry_requestUnit() { g_unit_pending = true; }
void Telemetry_requestEqu()  { g_equ_pending  = true; }
void Telemetry_requestBit()  { g_bit_pending  = true; }

void Telemetry_drainBursts() {
  if (g_parm_pending) {
    Serial1.print(F(":AB1JC-11 :PARM.TxTmp,ITmp1,ITmp2,OTemp,Vspd,GPSok,Time,COM,Alt,Mok,PRE,UP,DN\n"));
    g_parm_pending = false;
  }
  if (g_unit_pending) {
    Serial1.print(F(":AB1JC-11 :UNIT.deg.C,deg.C,deg.C,deg.C,ft/s,valid,sync,OK,CAL\n"));
    g_unit_pending = false;
  }
  if (g_equ_pending) {
    Serial1.print(F(":AB1JC-11 :EQNS.0,1,-100,0,1,-100,0,1,-100,0,1,-100,0,1,-200\n"));
    g_equ_pending = false;
  }
  if (g_bit_pending) {
    Serial1.print(F(":AB1JC-11 :BITS.11111111, Univ.Bridgeport USIP Balloon\n"));
    g_bit_pending = false;
  }
}

// -- Operator-command responses ----------------------------------------------

void Telemetry_processIdent() {
  Serial1.print(">Bat1:");      Serial1.print(g_state.bat_v1);
  Serial1.print(" Bat2:");      Serial1.print(g_state.bat_v2);
  Serial1.print(" P_alt:");     Serial1.print(g_state.pressure_alt);
  Serial1.print(" Pres:");      Serial1.print(g_state.ps_pressure, 0);
  Serial1.println(" Univ.Bridgeport USIP Balloon");

  Serial.print(">Bat1:");       Serial.print(g_state.bat_v1);
  Serial.print(" Bat2:");       Serial.print(g_state.bat_v2);
  Serial.print(" P_ALT:");      Serial.print(g_state.pressure_alt, 0);
  Serial.print(" Pres:");       Serial.print(g_state.ps_pressure, 0);
  Serial.println(" USIP Balloon");
}

void Telemetry_processMxAlt() {
  Serial1.print("New Max Alt = ");
  Serial1.print(g_state.max_alt, 0);
  Serial1.println(",000 ft.");
  Serial.print("New Max Alt = ");
  Serial.print(g_state.max_alt, 0);
  Serial.println(",000 ft.");
}
