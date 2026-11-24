// BTLink.h
// All BluetoothSerial access. Encapsulates the HC-05 master pairing,
// line-buffered RX framer, MONKEY-reply parser, periodic reconnect
// (bug #8 fix), and the STUB_PEER bench-test mode.
//
// On ESP-32, the Arduino runtime does NOT auto-dispatch a serialEventBT
// for BluetoothSerial — `BT_pollRx()` must be called from loop() every
// iteration.

#ifndef USIP_BTLINK_H
#define USIP_BTLINK_H

#include <stdint.h>

// One-time init. In STUB_PEER mode, no real BT begins; g_state.bt is set
// to BT_CONNECTED immediately so monkey traffic flows synthetically.
void BT_init();

// RX poll. Drains SerialBT (or the synth queue under STUB_PEER), framing
// on '\n' and dispatching complete MONKEY replies. Cheap when no bytes.
void BT_pollRx();

// 5-second reconnect tick. No-op when already connected or in STUB_PEER.
// Exponential backoff up to 60 s when peer is unreachable.
void BT_reconnectTick();

// 30-second poll: send "MONKEY 12" if monkey is OK and not busy.
void BT_pollMonkeyOk();

// 20-second poll: send "MONKEY 09" if monkey is unknown/down.
void BT_pollMonkeyDown();

// Send "MONKEY XX\n" to the peer (or queue a synthetic reply under
// STUB_PEER). Returns true on success.
bool BT_sendMonkey(uint8_t cmd);

#endif  // USIP_BTLINK_H
