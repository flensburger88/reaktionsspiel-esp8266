#pragma once
#include "protocol.h"

// Game master. Runs only on the elected gateway unit.
// It only runs games; competitions and rankings live in the phone app.
namespace gateway
{
  enum Mode : uint8_t
  {
    MODE_SOLO, // one player, score = sum of reaction times + penalties
    MODE_DUEL, // two players at once (red/blue), first to finish wins
  };

  struct StartRequest
  {
    Mode mode;
    const char *players[LANES]; // players[1] only for a duel
    int presses;
    int timeoutMs;
    uint32_t bestMs;  // solo: competition best so far, 0 = none (rainbow on a new best)
    const char *comp; // competition id from the app, echoed in the result
  };

  void begin();
  void onMsg(const Msg &m); // HELLO / PRESSED / FALSE_PRESS from units (incl. itself)
  void update();

  // commands from the web app; on failure error says why
  bool startGame(const StartRequest &req, String &error);
  void abortGame();
  bool identify(int unitNumber);

  String stateJson();
  String resultsJson(); // finished games still kept here, newest last
}
