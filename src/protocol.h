#pragma once
#include <Arduino.h>

// ESP-NOW message format. Everything is broadcast; the gateway re-sends its
// game state every STATE_INTERVAL_MS, so a lost packet only delays things.

constexpr uint8_t MSG_MAGIC = 0xB5;
constexpr uint8_t MSG_VERSION = 3; // v2: two lanes (duel), v3: test mode
constexpr uint8_t LANES = 2;       // lane 0 = red player, lane 1 = blue player (duel only)

enum MsgType : uint8_t
{
  MSG_STATE = 1,       // gateway -> all: current phase / targets
  MSG_HELLO = 2,       // unit -> gateway: "I am here"
  MSG_PRESSED = 3,     // unit -> gateway: lane/seq[0] pressed after value ms, resent until the state moves on
  MSG_FALSE_PRESS = 4, // unit -> gateway: pressed while not lit, seq[0] = per-unit counter
  MSG_TEST_PRESS = 5,  // unit -> gateway: pressed in test mode, seq[0] = per-unit counter
};

enum Phase : uint8_t
{
  PH_IDLE = 0,
  PH_COUNTDOWN = 1, // value = ms until the start
  PH_RUNNING = 2,   // target[lane] is lit (all zero = none), seq[lane] = presses done
  PH_RESULT = 3,    // value = ResultShow
  PH_IDENTIFY = 4,  // target[0] blinks so the player sees which unit it is
  PH_TEST = 5,      // hardware test on all units until switched off
};

enum ResultShow : uint8_t
{
  SHOW_NORMAL = 0,
  SHOW_BEST = 1,   // solo: new best time in this competition
  SHOW_LANE_A = 2, // duel: red won
  SHOW_LANE_B = 3, // duel: blue won
};

struct __attribute__((packed)) Msg
{
  uint8_t magic;
  uint8_t version;
  uint8_t type;
  uint8_t phase;
  uint8_t gameId;
  uint8_t lane; // MSG_PRESSED: which lane was pressed
  uint8_t seq[LANES];
  uint8_t from[6]; // sender station MAC
  uint8_t target[LANES][6];
  uint32_t value;
};

inline bool sameMac(const uint8_t *a, const uint8_t *b)
{
  return memcmp(a, b, 6) == 0;
}

inline String macToString(const uint8_t *mac)
{
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
