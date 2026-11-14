#include <Arduino.h>
#include "config.h"
#include "gateway.h"
#include "leds.h"
#include "radio.h"
#include "unit.h"
#include "web.h"

// All units run this firmware. At boot a unit listens for a gateway; if it
// hears none it becomes the gateway itself (own WiFi + web app + game master).
// If two gateways ever see each other, the one with the higher MAC restarts.

enum Role
{
  ELECTING,
  ROLE_UNIT,
  ROLE_GATEWAY
};

Role role = ELECTING;
uint32_t electionUntil = 0;

void sendFromUnit(Msg &m)
{
  if (role == ROLE_GATEWAY)
  {
    memcpy(m.from, radio::selfMac(), 6);
    gateway::onMsg(m);
  }
  else
    radio::send(m);
}

void becomeUnit()
{
  role = ROLE_UNIT;
  Serial.println("Role: unit");
  unit::begin(false, sendFromUnit);
}

void becomeGateway()
{
  role = ROLE_GATEWAY;
  Serial.println("Role: gateway");
  radio::begin(true);
  unit::begin(true, sendFromUnit);
  web::begin();
  gateway::begin();
}

void handle(const Msg &m)
{
  switch (role)
  {
  case ELECTING:
    if (m.type == MSG_STATE)
    {
      becomeUnit();
      unit::onState(m);
    }
    break;
  case ROLE_UNIT:
    if (m.type == MSG_STATE)
      unit::onState(m);
    break;
  case ROLE_GATEWAY:
    if (m.type == MSG_STATE)
    {
      if (memcmp(m.from, radio::selfMac(), 6) < 0)
      {
        Serial.println("Other gateway wins, restarting as unit");
        delay(100);
        ESP.restart();
      }
    }
    else
      gateway::onMsg(m);
    break;
  }
}

// Serial commands for testing without buttons / phone:
//   press                          any role: simulate a button press
//   gateway                        become the gateway (lower MAC wins against an existing one)
//   start <name> [presses] [ms]    gateway: start a solo game
//   duel <red> <blue> [presses] [ms]   gateway: start a duel
//   abort | state | results | identify <n>   gateway only
void handleCommand(String line)
{
  line.trim();
  int sp = line.indexOf(' ');
  String cmd = sp < 0 ? line : line.substring(0, sp);
  String args = sp < 0 ? "" : line.substring(sp + 1);

  if (cmd == "press")
  {
    if (role != ELECTING)
      unit::simulatePress();
    return;
  }
  if (cmd == "gateway")
  {
    // take over; if another gateway has a lower MAC, it wins and we restart
    if (role != ROLE_GATEWAY)
      becomeGateway();
    return;
  }
  if (role != ROLE_GATEWAY)
  {
    Serial.println("Not the gateway");
    return;
  }
  if (cmd == "start" || cmd == "duel")
  {
    char a[MAX_PLAYER_NAME + 1] = "A", b[MAX_PLAYER_NAME + 1] = "B";
    int presses = DEFAULT_PRESSES, timeout = DEFAULT_TIMEOUT_MS;
    gateway::StartRequest req{};
    if (cmd == "duel")
    {
      sscanf(args.c_str(), "%16s %16s %d %d", a, b, &presses, &timeout);
      req.mode = gateway::MODE_DUEL;
    }
    else
    {
      sscanf(args.c_str(), "%16s %d %d", a, &presses, &timeout);
      req.mode = gateway::MODE_SOLO;
    }
    req.players[0] = a;
    req.players[1] = b;
    req.presses = presses;
    req.timeoutMs = timeout;
    req.comp = "serial";
    String error;
    if (!gateway::startGame(req, error))
      Serial.println("Cannot start: " + error);
  }
  else if (cmd == "abort")
    gateway::abortGame();
  else if (cmd == "state")
    Serial.println(gateway::stateJson());
  else if (cmd == "results")
    Serial.println(gateway::resultsJson());
  else if (cmd == "identify")
    gateway::identify(args.toInt());
  else
    Serial.println("Unknown command");
}

void readSerial()
{
  static String line;
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == '\n' || c == '\r')
    {
      if (line.length())
        handleCommand(line);
      line = "";
    }
    else if (line.length() < 64)
      line += c;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\nButtonSpielV3 starting");
  Serial.printf("Reset reason: %s\n", ESP.getResetInfo().c_str());
  randomSeed(RANDOM_REG32);
  leds::begin();
  leds::setPattern(leds::ELECTION);

  if (!radio::begin(false))
    Serial.println("ESP-NOW init failed");
  Serial.printf("MAC %s\n", macToString(radio::selfMac()).c_str());
  electionUntil = millis() + ELECTION_MIN_MS + random(ELECTION_RANDOM_MS);
}

void loop()
{
  Msg m;
  while (radio::receive(m))
    handle(m);

  switch (role)
  {
  case ELECTING:
    if ((int32_t)(millis() - electionUntil) >= 0)
      becomeGateway();
    break;
  case ROLE_UNIT:
    unit::update();
    // gateway gone for good (switched off): restart and elect a new one
    if (unit::msSinceState() > GATEWAY_REELECT_MS)
    {
      Serial.println("No gateway, restarting to re-elect");
      ESP.restart();
    }
    break;
  case ROLE_GATEWAY:
    unit::update();
    gateway::update();
    web::update();
    break;
  }
  readSerial();
  leds::update();

  // the game must never stall; log it if something blocks the loop
  static uint32_t lastLoop = 0, lastStallLog = 0;
  uint32_t now = millis();
  if (lastLoop && now - lastLoop > 100 && now - lastStallLog > 1000)
  {
    lastStallLog = now;
    Serial.printf("Loop stalled %lu ms\n", (unsigned long)(now - lastLoop));
  }
  lastLoop = now;
}
