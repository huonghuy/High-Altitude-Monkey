// GPS.cpp
//
// char-buffer NMEA parser. Replaces the String-based ProcessOwnGPS() and
// serialEvent1() in the original .ino. No heap, fixed-size buffer.

#include "GPS.h"
#include "Config.h"
#include "State.h"
#include "Time.h"
#include "Phase.h"
#include "Commands.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

namespace {
  constexpr size_t GPS_BUF_LEN = 128;
  char    g_buf[GPS_BUF_LEN];
  size_t  g_buf_len = 0;
  bool    g_dbg     = true;

  // Reset the framer.
  inline void resetBuf() { g_buf_len = 0; g_buf[0] = 0; }

  // Validate the trailing '*HH' checksum of a NUL-terminated NMEA sentence
  // that begins with '$'. Returns the offset of '*' on success, -1 on bad/
  // missing checksum or malformed input.
  int validateChecksum(const char* s, size_t len) {
    // Find the LAST '*'. NMEA sentences have only one, but malformed input
    // might have payload chars that look similar — be conservative.
    int star = -1;
    for (size_t i = 0; i < len; ++i) if (s[i] == '*') star = (int)i;
    if (star < 0 || (size_t)(star + 2) >= len) return -1;

    auto hexNibble = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
      if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
      return -1;
    };
    int hi = hexNibble(s[star + 1]);
    int lo = hexNibble(s[star + 2]);
    if (hi < 0 || lo < 0) return -1;
    uint8_t expected = (uint8_t)((hi << 4) | lo);

    uint8_t got = 0;
    // XOR everything between '$' (exclusive) and '*' (exclusive).
    for (int i = 1; i < star; ++i) got ^= (uint8_t)s[i];
    return (got == expected) ? star : -1;
  }

  // Parse $GPGGA. Caller has already verified the checksum.
  // Format: $GPGGA,hhmmss.ss,lat,N,lon,E,fix,nsat,hdop,alt,M,...
  // We extract: hr/min/sec, lat/lon (informational only — not stored
  // beyond debug), fix quality, alt + units.
  void parseGGA(const char* s, size_t len, uint32_t at_ms) {
    // Walk to the 6th comma (start of fix quality). Fields:
    //   0=$GPGGA, 1=time, 2=lat, 3=N/S, 4=lon, 5=E/W, 6=fix, 7=nsat,
    //   8=hdop, 9=alt, 10=units, ...
    const char* p = s;
    int comma_count = 0;
    const char* fields[12] = { 0 };
    fields[0] = p;
    while (*p && comma_count < 11) {
      if (*p == ',') {
        comma_count++;
        fields[comma_count] = p + 1;
      }
      p++;
    }
    if (comma_count < 10) return;  // not enough fields

    // Fix quality (field 6). 0 = invalid, 1 = GPS, 2 = DGPS.
    if (fields[6] == nullptr || fields[6][0] == '0' || fields[6][0] == ',') {
      return;
    }

    // Time field is fields[1], format "hhmmss.ss"
    if (fields[1] && (size_t)(fields[2] - fields[1]) >= 7) {
      char hh[3] = { fields[1][0], fields[1][1], 0 };
      char mm[3] = { fields[1][2], fields[1][3], 0 };
      char ss[3] = { fields[1][4], fields[1][5], 0 };
      int gh = atoi(hh), gm = atoi(mm), gs = atoi(ss);

      // Altitude field 9, units field 10.
      char alt_buf[16] = { 0 };
      size_t alt_len = (size_t)(fields[10] - fields[9]) - 1;
      if (alt_len >= sizeof(alt_buf)) alt_len = sizeof(alt_buf) - 1;
      memcpy(alt_buf, fields[9], alt_len);
      alt_buf[alt_len] = 0;
      float gps_in = (float)atof(alt_buf);
      char units = fields[10] ? fields[10][0] : '?';

      // Roll the alt history (t-15, t-30, t-45) before storing the new value.
      g_state.gps_alt3 = g_state.gps_alt2;
      g_state.gps_alt2 = g_state.gps_alt1;
      g_state.gps_alt1 = g_state.gps_alt;

      if (gps_in != 0.0f) {
        if      (units == 'M') g_state.gps_alt = gps_in * ALT_M_TO_FT;
        else if (units == 'F') g_state.gps_alt = gps_in;
        // Else: bad units, leave gps_alt unchanged.
      }

      g_state.gps = GPS_VALID;
      Time_onGpsFix(gh, gm, gs, at_ms);

      if (g_dbg) {
        Serial.print("GGA OK alt=");
        Serial.print(g_state.gps_alt);
        Serial.print(" units=");
        Serial.println(units);
      }

      // Hand off to Phase module for abort-altitude debounce.
      Phase_onGpsAlt();
    }
  }

  // Parse $GPRMC. Only purpose: extract date string into a debug print.
  // Format: $GPRMC,time,A/V,lat,N/S,lon,E/W,speed,course,date,...
  void parseRMC(const char* s, size_t len) {
    // Walk to field 9 (date) — that's 9 commas in.
    const char* p = s;
    int commas = 0;
    while (*p && commas < 9) { if (*p == ',') commas++; p++; }
    if (commas < 9) return;
    // Validity field is at position 2 (1 comma in), 'A' = valid.
    const char* p2 = s;
    int c2 = 0;
    while (*p2 && c2 < 2) { if (*p2 == ',') c2++; p2++; }
    if (c2 < 2 || *p2 != 'A') return;

    if (g_dbg) {
      Serial.print("RMC date=");
      while (*p && *p != ',') { Serial.print(*p); p++; }
      Serial.println();
    }
  }

  // Process a complete framed line (without trailing newline).
  void process(const char* s, size_t len, uint32_t at_ms) {
    // Find first '$' in the buffer (the source can stutter).
    const char* dollar = (const char*)memchr(s, '$', len);
    if (!dollar) {
      Serial1.println("USIP: Invalid Command No $");
      if (g_dbg) {
        Serial.print("Bad: ");
        Serial.println(s);
      }
      return;
    }
    size_t dlen = len - (dollar - s);
    if (dlen < 6) return;  // need at least "$XXXXX"

    // Identify the message type by chars 1..5.
    if (memcmp(dollar + 1, "GPGGA", 5) == 0) {
      int star = validateChecksum(dollar, dlen);
      if (star < 0) {
        if (g_dbg) Serial.println("Bad checksum");
        g_state.gps = GPS_INVALID;
        return;
      }
      parseGGA(dollar, dlen, at_ms);
    } else if (memcmp(dollar + 1, "GPRMC", 5) == 0) {
      parseRMC(dollar, dlen);
    } else {
      // Non-NMEA radio command. Hand off to Commands.
      Commands_dispatchRadioLine(dollar, dlen);
    }
  }
}

void GPS_pollRx() {
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (g_buf_len + 1 >= GPS_BUF_LEN) {
      // Overflow: drop the line.
      resetBuf();
      continue;
    }
    g_buf[g_buf_len++] = c;
    g_buf[g_buf_len]   = 0;
    if (c == '\n') {
      // Frame complete. Strip trailing CR/LF for cleaner parsing.
      size_t plen = g_buf_len;
      while (plen > 0 && (g_buf[plen - 1] == '\n' || g_buf[plen - 1] == '\r')) plen--;
      g_buf[plen] = 0;
      if (plen >= 6) process(g_buf, plen, millis());
      resetBuf();
    }
  }
}
