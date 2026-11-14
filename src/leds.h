#pragma once
#include <FastLED.h>

namespace leds
{
  enum Pattern
  {
    OFF,
    ELECTION,     // dim white: looking for a gateway
    NO_GATEWAY,   // yellow blink: unit lost the gateway
    IDLE_UNIT,    // blue breathing
    IDLE_GATEWAY, // cyan breathing
    COUNTDOWN,    // white blink once per second
    TARGET_A,     // red chase: press me! (solo, duel red player)
    TARGET_B,     // blue chase: press me! (duel blue player)
    RESULT,       // purple pulse
    RESULT_BEST,  // rainbow: new best time
    RESULT_A,     // red pulse: red player won the duel
    RESULT_B,     // blue pulse: blue player won the duel
    IDENTIFY,     // fast blue blink
    TEST,         // hardware test: red, green, blue, white, then one LED after the other
  };

  void begin();
  void setPattern(Pattern p);         // renders immediately when changed
  void flash(CRGB color, uint16_t ms); // short overlay over the pattern
  void update();                      // call every loop
}
