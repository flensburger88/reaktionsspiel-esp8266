#pragma once
#include <Arduino.h>

// Phone web app: own WiFi network (softAP), captive DNS, page and WebSocket (/ws) on :80.
namespace web
{
  void begin();
  void update();
  void broadcast(const String &json); // to all connected phones
}
