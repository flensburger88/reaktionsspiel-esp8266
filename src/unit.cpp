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

    // turn that lights at lightAt: announced by the gateway (target + delay)
    // or started by ourselves when we are next and hear the previous press
    bool scheduled = false;
    Turn schedTurn{};
    uint32_t lightAt = 0;

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

    bool isSelf(const uint8_t *mac)
    {
      return sameMac(mac, radio::selfMac());
    }

    // Is a turn we lit or scheduled still ours according to the gateway?
    // The gateway may lag behind (it has not heard the previous press yet),
    // then it still lists us as next for the lane.
    bool stillOurs(const Turn &t)
    {
      if (!gameRunning() || state.gameId != t.game)
        return false;
      uint8_t seq = state.seq[t.lane];
      if (seq == t.seq)
        return isSelf(state.target[t.lane]);
      if ((uint8_t)(seq + 1) == t.seq)
        return isSelf(state.next[t.lane]);
      return false;
    }

    void schedule(const Turn &t, uint32_t at)
    {
      if ((hasDone && doneTurn == t) || (active && activeTurn == t))
        return;
      if (scheduled && schedTurn == t)
      {
        if ((int32_t)(at - lightAt) < 0)
          lightAt = at; // gateway and previous unit both told us, the earlier wins
        return;
      }
      if (active || scheduled)
        return; // busy with the other lane (duel); the gateway sorts this out
      scheduled = true;
      schedTurn = t;
      lightAt = at;
    }

    void tryLight(uint32_t now)
    {
      if (!scheduled || (int32_t)(now - lightAt) < 0)
        return;
      scheduled = false;
      active = true;
      activeTurn = schedTurn;
      activatedAt = now;
      Serial.printf("Lit turn %u lane %c\n", activeTurn.seq + 1, 'A' + activeTurn.lane); // tools/autoplay.py presses on this
      showPhase();
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
        // we may be next ourselves (duel with two units); our own broadcast does not come back
        uint8_t l = activeTurn.lane;
        if (state.seq[l] == activeTurn.seq && isSelf(state.next[l]))
          schedule(Turn{activeTurn.game, l, (uint8_t)(activeTurn.seq + 1)}, now + state.nextDelay[l]);
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
    uint32_t now = millis();
    stateAt = now;

    // drop what the gateway gave to someone else, timed out or never knew
    if (active && !stillOurs(activeTurn))
      active = false;
    if (scheduled && !stillOurs(schedTurn))
      scheduled = false;
    // resend our press until the gateway has counted it (or timed the turn out);
    // it may still be one press behind when we were lit by the previous unit
    if (pending && !(gameRunning() && s.gameId == doneTurn.game && s.seq[doneTurn.lane] <= doneTurn.seq))
      pending = false;

    // the gateway never lights one unit for both lanes at once
    if (s.phase == PH_RUNNING)
      for (uint8_t l = 0; l < LANES; l++)
        if (isSelf(s.target[l]))
          schedule(Turn{s.gameId, l, s.seq[l]}, now + s.delay[l]);
    tryLight(now);
    showPhase();
  }

  void onPeerPressed(const Msg &m)
  {
    // the press of the unit before us: start our pause right away instead of
    // waiting for the gateway to hear it and announce us (one radio hop less,
    // and the resends of the press give us more chances than the gateway's state)
    uint8_t l = m.lane;
    if (!gameRunning() || l >= LANES || m.gameId != state.gameId || m.seq[0] != state.seq[l])
      return;
    if (!isSelf(state.next[l]) || !sameMac(m.from, state.target[l]))
      return;
    uint32_t now = millis();
    uint16_t age = m.age < state.nextDelay[l] ? m.age : state.nextDelay[l];
    schedule(Turn{m.gameId, l, (uint8_t)(m.seq[0] + 1)}, now - age + state.nextDelay[l]);
    tryLight(now);
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
        scheduled = false;
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

    tryLight(now);

    if (pending)
    {
      if (now - pendingSince > PRESS_RESEND_MAX_MS)
        pending = false;
      else if (now - lastResend >= PRESS_RESEND_MS)
      {
        lastResend = now;
        uint32_t age = now - pendingSince;
        pressedMsg.age = age > 0xFFFF ? 0xFFFF : age;
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
