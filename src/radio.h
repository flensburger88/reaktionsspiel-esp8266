#pragma once
#include "protocol.h"

// Thin ESP-NOW wrapper: broadcast only, received messages are queued for loop().
namespace radio
{
  bool begin(bool gateway); // (re)initialises WiFi mode + ESP-NOW
  void send(Msg &m);        // fills magic/version/from and broadcasts
  bool receive(Msg &out);   // next queued message, false if none
  const uint8_t *selfMac();
}
