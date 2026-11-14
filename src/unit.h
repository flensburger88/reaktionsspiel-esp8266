#pragma once
#include "protocol.h"

// Button unit logic. Runs on every device, the gateway included.
namespace unit
{
  typedef void (*SendFn)(Msg &m); // delivers a unit message to the gateway

  void begin(bool isGateway, SendFn send);
  void onState(const Msg &state); // game state from the gateway (radio or local)
  void update();                  // button, resends, hello, gateway watchdog
  uint32_t msSinceState();        // how long the gateway has been silent
  void simulatePress();           // serial "press" command, for testing without a button
}
