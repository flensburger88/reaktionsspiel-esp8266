#include "unit.h"
#include "config.h"
#include "leds.h"
#include "radio.h"

namespace unit
{
  namespace
  {
    SendFn sendFn = nullptr;
    bool gatewayRole = false;

    Msg state{};
    bool haveState = false;
    uint32_t stateAt = 0;

    // a turn is identified by game, lane and press number
    struct Turn
    {
      uint8_t game, lane, seq;
      bool operator==(const Turn &o) const { return game == o.game && lane == o.lane && seq == o.seq; }
    };

    // currently lit turn
    bool active = false;
    Turn activeTurn{};
    uint32_t activatedAt = 0;

    // last pressed turn, so a repeated state does not light us again
    bool hasDone = false;
    Turn doneTurn{};

    // PRESSED is resent until the gateway moves on
    bool pending = false;
    uint32_t pendingSince = 0, lastResend = 0;
    Msg pressedMsg{};

    // FALSE_PRESS is sent a few times, the gateway de-duplicates by seq
    uint8_t falsePressCounter = 0;
    uint8_t falseRepeats = 0;
    uint32_t lastFalseSend = 0;
    Msg falseMsg{};

    bool rawDown = false;
    uint32_t rawChangeAt = 0;
    uint32_t lastHello = 0;

    bool gameRunning()
    {
      return haveState && state.phase == PH_RUNNING;
    }

    leds::Pattern idlePattern()
    {
      return gatewayRole ? leds::IDLE_GATEWAY : leds::IDLE_UNIT;
    }

    void showPhase()
    {
      switch (state.phase)
      {
      case PH_IDLE:
        leds::setPattern(idlePattern());
        break;
      case PH_COUNTDOWN:
        leds::setPattern(leds::COUNTDOWN);
        break;
      case PH_RUNNING:
        leds::setPattern(!active ? leds::OFF : activeTurn.lane == 0 ? leds::TARGET_A : leds::TARGET_B);
        break;
      case PH_RESULT:
        leds::setPattern(state.value == SHOW_BEST     ? leds::RESULT_BEST
                         : state.value == SHOW_LANE_A ? leds::RESULT_A
                         : state.value == SHOW_LANE_B ? leds::RESULT_B
                                                      : leds::RESULT);
        break;
      case PH_IDENTIFY:
        leds::setPattern(sameMac(state.target[0], radio::selfMac()) ? leds::IDENTIFY : idlePattern());
        break;
      case PH_TEST:
        leds::setPattern(leds::TEST);
        break;
      }
    }

    void onPress(uint32_t now)
    {
      if (active)
      {
        // reaction time is measured here, so radio latency does not count
        uint32_t reaction = now - activatedAt;
        active = false;
        hasDone = true;
        doneTurn = activeTurn;

        pressedMsg = Msg{};
        pressedMsg.type = MSG_PRESSED;
        pressedMsg.gameId = activeTurn.game;
        pressedMsg.lane = activeTurn.lane;
        pressedMsg.seq[0] = activeTurn.seq;
        pressedMsg.value = reaction;
        pending = true;
        pendingSince = now;
        lastResend = now;

        Serial.printf("Pressed lane %c turn %u after %lu ms\n", 'A' + activeTurn.lane, activeTurn.seq + 1, (unsigned long)reaction);
        leds::flash(CRGB::Green, 150);
        leds::setPattern(leds::OFF);
        Msg copy = pressedMsg;
        sendFn(copy); // may call onState() synchronously on the gateway
      }
      else if (haveState && state.phase == PH_TEST)
      {
        // the app counts test presses per unit, so the player sees every button arrive
        falseMsg = Msg{};
        falseMsg.type = MSG_TEST_PRESS;
        falseMsg.seq[0] = ++falsePressCounter;
        falseRepeats = 3;
        lastFalseSend = 0;
        Serial.println("Test press");
        leds::flash(CRGB::White, 300);
      }
      else if (gameRunning())
      {
        falseMsg = Msg{};
        falseMsg.type = MSG_FALSE_PRESS;
        falseMsg.gameId = state.gameId;
        falseMsg.seq[0] = ++falsePressCounter;
        falseRepeats = 3;
        lastFalseSend = 0;
        Serial.println("False press");
        leds::flash(CRGB(255, 80, 0), 300);
      }
    }
  }

  void begin(bool isGateway, SendFn send)
  {
    gatewayRole = isGateway;
    sendFn = send;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    rawDown = digitalRead(BUTTON_PIN) == LOW;
    rawChangeAt = millis();
  }

  void onState(const Msg &s)
  {
    state = s;
    haveState = true;
    stateAt = millis();

    // the gateway never lights one unit for both lanes at once
    int lane = -1;
    if (s.phase == PH_RUNNING)
      for (uint8_t l = 0; l < LANES; l++)
        if (sameMac(s.target[l], radio::selfMac()))
          lane = l;

    if (lane >= 0)
    {
      Turn turn{s.gameId, (uint8_t)lane, s.seq[lane]};
      if (!(hasDone && doneTurn == turn) && !(active && activeTurn == turn))
      {
        active = true;
        activeTurn = turn;
        activatedAt = millis();
        Serial.printf("Lit turn %u lane %c\n", turn.seq + 1, 'A' + lane); // tools/autoplay.py presses on this
      }
      if (pending && !(doneTurn == turn))
        pending = false; // lit for a new turn, so the old press arrived
    }
    else
    {
      // the gateway moved on: our press arrived or the turn timed out
      active = false;
      pending = false;
    }
    showPhase();
  }

  uint32_t msSinceState()
  {
    return millis() - stateAt;
  }

  void simulatePress()
  {
    onPress(millis());
  }

  void update()
  {
    uint32_t now = millis();

    if (!gatewayRole)
    {
      if (now - lastHello >= HELLO_INTERVAL_MS)
      {
        lastHello = now;
        Msg hello{};
        hello.type = MSG_HELLO;
        sendFn(hello);
      }
      if (haveState && now - stateAt > GATEWAY_LOST_MS)
      {
        Serial.println("Gateway lost");
        haveState = false;
        active = false;
        pending = false;
      }
      if (!haveState)
        leds::setPattern(leds::NO_GATEWAY);
    }

    // A press is a LOW edge after the button was released for DEBOUNCE_MS;
    // this ignores bounces on press and release without delaying the press.
    bool down = digitalRead(BUTTON_PIN) == LOW;
    if (down != rawDown)
    {
      uint32_t stableFor = now - rawChangeAt;
      rawDown = down;
      rawChangeAt = now;
      if (down && stableFor >= DEBOUNCE_MS)
        onPress(now);
    }

    if (pending)
    {
      if (now - pendingSince > PRESS_RESEND_MAX_MS)
        pending = false;
      else if (now - lastResend >= PRESS_RESEND_MS)
      {
        lastResend = now;
        Msg copy = pressedMsg;
        sendFn(copy);
      }
    }

    if (falseRepeats > 0 && now - lastFalseSend >= 30)
    {
      falseRepeats--;
      lastFalseSend = now;
      Msg copy = falseMsg;
      sendFn(copy);
    }
  }
}
