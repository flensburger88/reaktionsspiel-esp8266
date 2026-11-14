#include "gateway.h"
#include "config.h"
#include "radio.h"
#include "unit.h"
#include "web.h"
#include <ArduinoJson.h>

namespace gateway
{
  namespace
  {
    struct Unit
    {
      bool used;
      uint8_t mac[6];
      uint32_t lastSeen;
      uint8_t lastFalseSeq;
    };

    // One player's progress. Solo uses lane 0 only.
    struct Lane
    {
      char player[MAX_PLAYER_NAME + 1];
      uint8_t seq;        // presses done
      int8_t target;      // lit unit, -1 = none
      int8_t lastTarget;
      uint32_t litSince;  // gateway time the target was lit
      uint32_t nextAt;    // when to light the next target
      uint16_t times[MAX_PRESSES];
      bool missed[MAX_PRESSES];
      uint8_t falsePresses; // solo only: in a duel nobody knows who pressed
      bool done;
      uint32_t finishMs; // ms from start until the last press
    };

    Unit units[MAX_UNITS]; // index 0 = the gateway itself

    Phase phase = PH_IDLE;
    Mode mode = MODE_SOLO;
    uint8_t laneCount = 1;
    Lane lanes[LANES];
    uint8_t gameId = 0;
    uint8_t presses = DEFAULT_PRESSES;
    uint16_t timeoutMs = DEFAULT_TIMEOUT_MS;
    uint32_t bestMs = 0;
    char comp[MAX_COMP_ID + 1] = "";
    uint8_t strayPresses = 0; // duel: presses on unlit buttons
    uint32_t raceStart = 0;
    uint32_t phaseUntil = 0;
    uint8_t resultShow = SHOW_NORMAL;
    int8_t identifyTarget = -1;

    // finished games as JSON, kept until the phone has stored them
    uint32_t bootId = 0;
    String results[MAX_RESULTS];
    uint8_t resultCount = 0;
    String lastResultId;

    uint32_t lastStateSent = 0;
    uint32_t lastWebPush = 0;

    bool online(int i, uint32_t now)
    {
      return units[i].used && (i == 0 || now - units[i].lastSeen < UNIT_OFFLINE_MS);
    }

    int onlineCount()
    {
      uint32_t now = millis();
      int n = 0;
      for (int i = 0; i < MAX_UNITS; i++)
        if (online(i, now))
          n++;
      return n;
    }

    const char *phaseName(Phase p)
    {
      switch (p)
      {
      case PH_COUNTDOWN:
        return "countdown";
      case PH_RUNNING:
        return "running";
      case PH_RESULT:
        return "result";
      case PH_IDENTIFY:
        return "identify";
      default:
        return "idle";
      }
    }

    void pushState()
    {
      lastWebPush = millis();
      web::broadcast(stateJson());
    }

    void sendState()
    {
      uint32_t now = millis();
      lastStateSent = now;
      Msg m{};
      m.type = MSG_STATE;
      m.phase = phase;
      m.gameId = gameId;
      for (uint8_t l = 0; l < LANES; l++)
      {
        m.seq[l] = lanes[l].seq;
        if (phase == PH_RUNNING && l < laneCount && lanes[l].target >= 0)
          memcpy(m.target[l], units[lanes[l].target].mac, 6);
      }
      if (phase == PH_IDENTIFY && identifyTarget >= 0)
        memcpy(m.target[0], units[identifyTarget].mac, 6);
      if (phase == PH_COUNTDOWN)
        m.value = (int32_t)(phaseUntil - now) > 0 ? phaseUntil - now : 0;
      else if (phase == PH_RESULT)
        m.value = resultShow;
      radio::send(m);
      unit::onState(m); // the gateway is a button unit too
    }

    void changed()
    {
      sendState();
      pushState();
    }

    void setPhase(Phase p, uint32_t durationMs)
    {
      phase = p;
      phaseUntil = millis() + durationMs;
      changed();
    }

    // Next unit for a lane: never the other player's lit unit, and not the
    // same unit twice in a row unless nothing else is free.
    int8_t pickTarget(uint8_t l)
    {
      uint32_t now = millis();
      int8_t candidates[MAX_UNITS];
      uint8_t count = 0;
      for (int i = 0; i < MAX_UNITS; i++)
      {
        bool otherLit = false;
        for (uint8_t o = 0; o < laneCount; o++)
          if (o != l && lanes[o].target == i)
            otherLit = true;
        if (online(i, now) && !otherLit)
          candidates[count++] = i;
      }
      if (count > 1)
      {
        uint8_t kept = 0;
        for (uint8_t i = 0; i < count; i++)
          if (candidates[i] != lanes[l].lastTarget)
            candidates[kept++] = candidates[i];
        count = kept;
      }
      return count ? candidates[random(count)] : -1;
    }

    void lightNext(uint8_t l)
    {
      Lane &lane = lanes[l];
      int8_t t = pickTarget(l);
      if (t < 0)
        return; // nothing free right now, try again next loop
      lane.target = t;
      lane.lastTarget = t;
      lane.litSince = millis();
      if (mode == MODE_SOLO)
        Serial.printf("Turn %u -> unit %d\n", lane.seq + 1, t + 1);
      else
        Serial.printf("Lane %c turn %u -> unit %d\n", 'A' + l, lane.seq + 1, t + 1);
      changed();
    }

    uint32_t laneTotal(const Lane &lane)
    {
      if (mode == MODE_DUEL)
        return lane.finishMs;
      uint32_t total = (uint32_t)lane.falsePresses * FALSE_PRESS_PENALTY_MS;
      for (uint8_t i = 0; i < presses; i++)
        total += lane.times[i];
      return total;
    }

    void finishGame()
    {
      int8_t winner = -1;
      if (mode == MODE_DUEL)
      {
        uint32_t a = laneTotal(lanes[0]), b = laneTotal(lanes[1]);
        winner = a < b ? 0 : b < a ? 1 : -1;
        resultShow = winner == 0 ? SHOW_LANE_A : winner == 1 ? SHOW_LANE_B : SHOW_NORMAL;
      }
      else
      {
        uint32_t total = laneTotal(lanes[0]);
        resultShow = (bestMs == 0 || total < bestMs) ? SHOW_BEST : SHOW_NORMAL;
      }

      char id[20];
      snprintf(id, sizeof(id), "%08lx-%u", (unsigned long)bootId, gameId);
      lastResultId = id;

      JsonDocument doc;
      doc["id"] = id;
      doc["comp"] = comp;
      doc["mode"] = mode == MODE_DUEL ? "duel" : "solo";
      doc["presses"] = presses;
      doc["timeoutMs"] = timeoutMs;
      doc["winner"] = winner;
      doc["stray"] = strayPresses;
      JsonArray ls = doc["lanes"].to<JsonArray>();
      for (uint8_t l = 0; l < laneCount; l++)
      {
        const Lane &lane = lanes[l];
        JsonObject o = ls.add<JsonObject>();
        o["player"] = lane.player;
        o["total"] = laneTotal(lane);
        o["finishMs"] = lane.finishMs;
        uint32_t sum = 0;
        uint8_t misses = 0;
        JsonArray t = o["times"].to<JsonArray>();
        for (uint8_t i = 0; i < presses; i++)
        {
          t.add(lane.times[i]);
          sum += lane.times[i];
          misses += lane.missed[i];
        }
        o["reactionSum"] = sum;
        o["misses"] = misses;
        o["falsePresses"] = lane.falsePresses;
      }
      String json;
      serializeJson(doc, json);

      if (resultCount == MAX_RESULTS)
      {
        for (uint8_t i = 1; i < MAX_RESULTS; i++)
          results[i - 1] = results[i];
        resultCount--;
      }
      results[resultCount++] = json;

      Serial.printf("Game over: %s\n", json.c_str());
      for (auto &lane : lanes)
        lane.target = -1;
      setPhase(PH_RESULT, RESULT_SHOW_MS);
      web::broadcast(String("{\"type\":\"result\",\"result\":") + json + "}");
    }

    void recordTurn(uint8_t l, uint32_t ms, bool miss)
    {
      Lane &lane = lanes[l];
      uint32_t now = millis();
      lane.times[lane.seq] = ms > 0xFFFF ? 0xFFFF : ms;
      lane.missed[lane.seq] = miss;
      lane.seq++;
      lane.target = -1;
      if (lane.seq >= presses)
      {
        lane.done = true;
        lane.finishMs = now - raceStart;
        if (mode == MODE_DUEL)
          Serial.printf("Lane %c finished after %lu ms\n", 'A' + l, (unsigned long)lane.finishMs);
      }
      else
        lane.nextAt = now + (mode == MODE_DUEL ? DUEL_PAUSE_MS : random(PAUSE_MIN_MS, PAUSE_MAX_MS + 1));

      bool allDone = true;
      for (uint8_t i = 0; i < laneCount; i++)
        allDone &= lanes[i].done;
      if (allDone)
        finishGame();
      else
        changed();
    }

    int findUnit(const uint8_t *mac)
    {
      for (int i = 0; i < MAX_UNITS; i++)
        if (units[i].used && sameMac(units[i].mac, mac))
          return i;
      return -1;
    }

    int registerUnit(const uint8_t *mac)
    {
      int i = findUnit(mac);
      if (i >= 0)
        return i;
      for (i = 1; i < MAX_UNITS; i++)
        if (!units[i].used)
        {
          units[i].used = true;
          memcpy(units[i].mac, mac, 6);
          units[i].lastFalseSeq = 0;
          Serial.printf("New unit %d: %s\n", i + 1, macToString(mac).c_str());
          return i;
        }
      return -1;
    }

    bool idle()
    {
      return phase == PH_IDLE || phase == PH_RESULT || phase == PH_IDENTIFY;
    }
  }

  void begin()
  {
    bootId = RANDOM_REG32;
    memset(units, 0, sizeof(units));
    memset(lanes, 0, sizeof(lanes));
    for (auto &lane : lanes)
      lane.target = lane.lastTarget = -1;
    units[0].used = true;
    memcpy(units[0].mac, radio::selfMac(), 6);
    sendState();
  }

  void onMsg(const Msg &m)
  {
    uint32_t now = millis();
    int u = findUnit(m.from);
    if (u < 0)
    {
      u = registerUnit(m.from);
      if (u < 0)
        return; // too many units
      units[u].lastSeen = now;
      pushState();
    }
    bool wasOffline = !online(u, now);
    units[u].lastSeen = now;
    if (wasOffline)
      pushState();

    if (phase != PH_RUNNING || m.gameId != gameId)
      return;
    switch (m.type)
    {
    case MSG_PRESSED:
      if (m.lane < laneCount && lanes[m.lane].target == u && m.seq[0] == lanes[m.lane].seq)
      {
        bool miss = m.value >= timeoutMs;
        recordTurn(m.lane, miss ? timeoutMs : m.value, miss);
      }
      break;
    case MSG_FALSE_PRESS:
      if (m.seq[0] != units[u].lastFalseSeq)
      {
        units[u].lastFalseSeq = m.seq[0];
        if (mode == MODE_SOLO)
          lanes[0].falsePresses++;
        else
          strayPresses++;
        Serial.printf("False press on unit %d\n", u + 1);
        pushState();
      }
      break;
    default:
      break;
    }
  }

  void update()
  {
    uint32_t now = millis();
    bool due = (int32_t)(now - phaseUntil) >= 0;
    switch (phase)
    {
    case PH_COUNTDOWN:
      if (due)
      {
        raceStart = now;
        for (uint8_t l = 0; l < laneCount; l++)
          lanes[l].nextAt = now + (mode == MODE_SOLO ? random(PAUSE_MIN_MS, PAUSE_MAX_MS + 1) : 0);
        setPhase(PH_RUNNING, 0);
      }
      break;
    case PH_RUNNING:
      for (uint8_t l = 0; l < laneCount && phase == PH_RUNNING; l++)
      {
        Lane &lane = lanes[l];
        if (lane.done)
          continue;
        if (lane.target < 0)
        {
          if ((int32_t)(now - lane.nextAt) >= 0)
            lightNext(l);
        }
        // the unit measures itself; this is only the fallback if it never answers
        else if (now - lane.litSince > (uint32_t)timeoutMs + 300)
        {
          Serial.printf("Lane %c turn %u timed out\n", 'A' + l, lane.seq + 1);
          recordTurn(l, timeoutMs, true);
        }
      }
      break;
    case PH_RESULT:
    case PH_IDENTIFY:
      if (due)
      {
        identifyTarget = -1;
        setPhase(PH_IDLE, 0);
      }
      break;
    default:
      break;
    }

    if (now - lastStateSent >= STATE_INTERVAL_MS)
      sendState();
    if (now - lastWebPush >= 1000)
      pushState(); // keeps the unit list and race clock in the app fresh
  }

  bool startGame(const StartRequest &req, String &error)
  {
    if (!idle())
    {
      error = "Es läuft schon ein Spiel";
      return false;
    }
    if (req.mode == MODE_DUEL && onlineCount() < 2)
    {
      error = "Für ein Duell müssen mindestens 2 Buttons online sein";
      return false;
    }
    mode = req.mode;
    laneCount = mode == MODE_DUEL ? 2 : 1;
    presses = constrain(req.presses, 1, MAX_PRESSES);
    timeoutMs = constrain(req.timeoutMs, MIN_TIMEOUT_MS, MAX_TIMEOUT_MS);
    bestMs = req.bestMs;
    strncpy(comp, req.comp ? req.comp : "", MAX_COMP_ID);
    comp[MAX_COMP_ID] = 0;
    memset(lanes, 0, sizeof(lanes));
    for (uint8_t l = 0; l < LANES; l++)
    {
      lanes[l].target = lanes[l].lastTarget = -1;
      const char *name = l < laneCount ? req.players[l] : nullptr;
      strncpy(lanes[l].player, name && *name ? name : "?", MAX_PLAYER_NAME);
    }
    strayPresses = 0;
    gameId++;
    for (int i = 0; i < MAX_UNITS; i++)
      units[i].lastFalseSeq = 0;
    if (mode == MODE_DUEL)
      Serial.printf("Start duel %u: %s vs %s, %u presses\n", gameId, lanes[0].player, lanes[1].player, presses);
    else
      Serial.printf("Start game %u for %s: %u presses\n", gameId, lanes[0].player, presses);
    setPhase(PH_COUNTDOWN, COUNTDOWN_MS);
    return true;
  }

  void abortGame()
  {
    if (phase == PH_IDLE)
      return;
    Serial.println("Game aborted");
    for (auto &lane : lanes)
      lane.target = -1;
    identifyTarget = -1;
    setPhase(PH_IDLE, 0);
  }

  bool identify(int unitNumber)
  {
    int i = unitNumber - 1;
    if (i < 0 || i >= MAX_UNITS || !units[i].used || !idle())
      return false;
    identifyTarget = i;
    setPhase(PH_IDENTIFY, IDENTIFY_MS);
    return true;
  }

  String stateJson()
  {
    uint32_t now = millis();
    JsonDocument doc;
    doc["type"] = "state";
    doc["phase"] = phaseName(phase);
    doc["mode"] = mode == MODE_DUEL ? "duel" : "solo";
    doc["comp"] = comp;
    doc["presses"] = presses;
    doc["timeoutMs"] = timeoutMs;
    doc["penaltyMs"] = FALSE_PRESS_PENALTY_MS;
    doc["stray"] = strayPresses;
    doc["raceMs"] = phase == PH_RUNNING ? now - raceStart : 0;
    doc["lastResult"] = lastResultId;
    if (phase != PH_IDLE)
    {
      JsonArray ls = doc["lanes"].to<JsonArray>();
      for (uint8_t l = 0; l < laneCount; l++)
      {
        const Lane &lane = lanes[l];
        JsonObject o = ls.add<JsonObject>();
        o["player"] = lane.player;
        o["seq"] = lane.seq;
        o["target"] = lane.target + 1; // unit number, 0 = none
        o["done"] = lane.done;
        o["finishMs"] = lane.finishMs;
        o["falsePresses"] = lane.falsePresses;
        JsonArray t = o["times"].to<JsonArray>();
        JsonArray miss = o["missed"].to<JsonArray>();
        for (uint8_t i = 0; i < lane.seq; i++)
        {
          t.add(lane.times[i]);
          miss.add(lane.missed[i]);
        }
      }
    }
    JsonArray us = doc["units"].to<JsonArray>();
    for (int i = 0; i < MAX_UNITS; i++)
    {
      if (!units[i].used)
        continue;
      JsonObject o = us.add<JsonObject>();
      o["n"] = i + 1;
      o["mac"] = macToString(units[i].mac);
      o["online"] = online(i, now);
      o["gateway"] = i == 0;
    }
    String out;
    serializeJson(doc, out);
    return out;
  }

  String resultsJson()
  {
    String out = "{\"type\":\"results\",\"list\":[";
    for (uint8_t i = 0; i < resultCount; i++)
    {
      if (i)
        out += ',';
      out += results[i];
    }
    out += "]}";
    return out;
  }
}
