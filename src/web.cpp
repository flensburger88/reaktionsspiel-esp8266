#include "web.h"
#include "config.h"
#include "gateway.h"
#include "protocol.h"
#include "webpage.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Asynchronous on purpose: a slow or sleeping phone must never block loop(),
// otherwise the game state stops going out and the units show "gateway lost".
// The async callbacks run in the network (SYS) context, so they only queue
// work; loop() handles commands and talks to the game.

namespace web
{
  namespace
  {
    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");
    DNSServer dns;
    bool started = false;

    struct Pending
    {
      uint32_t client;
      bool greet; // true: send state + scores, false: run command
      char text[200];
    };
    constexpr uint8_t QUEUE_SIZE = 8;
    Pending queue[QUEUE_SIZE];
    uint8_t head = 0, tail = 0;

    void enqueue(uint32_t client, bool greet, const uint8_t *data, size_t len)
    {
      uint8_t next = (head + 1) % QUEUE_SIZE;
      if (next == tail)
        return;
      Pending &p = queue[head];
      p.client = client;
      p.greet = greet;
      len = min(len, sizeof(p.text) - 1);
      memcpy(p.text, data, len);
      p.text[len] = 0;
      head = next;
    }

    void sendError(uint32_t client, const char *msg)
    {
      JsonDocument doc;
      doc["type"] = "error";
      doc["msg"] = msg;
      String out;
      serializeJson(doc, out);
      ws.text(client, out);
    }

    void runCommand(uint32_t client, const char *text)
    {
      JsonDocument doc;
      if (deserializeJson(doc, text))
        return;
      const char *cmd = doc["cmd"] | "";
      Serial.printf("Web command: %s\n", cmd);
      if (!strcmp(cmd, "start"))
      {
        // {"cmd":"start","mode":"solo"|"duel","players":["A","B"],"presses":10,"timeoutMs":5000,"bestMs":0,"comp":"id"}
        gateway::StartRequest req{};
        req.mode = !strcmp(doc["mode"] | "solo", "duel") ? gateway::MODE_DUEL : gateway::MODE_SOLO;
        req.players[0] = doc["players"][0] | "?";
        req.players[1] = doc["players"][1] | "?";
        req.presses = doc["presses"] | DEFAULT_PRESSES;
        req.timeoutMs = doc["timeoutMs"] | DEFAULT_TIMEOUT_MS;
        req.bestMs = doc["bestMs"] | 0;
        req.comp = doc["comp"] | "";
        String error;
        if (!gateway::startGame(req, error))
          sendError(client, error.c_str());
      }
      else if (!strcmp(cmd, "abort"))
        gateway::abortGame();
      else if (!strcmp(cmd, "identify"))
      {
        if (!gateway::identify(doc["unit"] | 0))
          sendError(client, "Gerade nicht möglich");
      }
    }

    void onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
      if (type == WS_EVT_CONNECT)
      {
        Serial.printf("WebSocket client %u connected\n", client->id());
        enqueue(client->id(), true, nullptr, 0);
      }
      else if (type == WS_EVT_DISCONNECT)
        Serial.printf("WebSocket client %u disconnected\n", client->id());
      else if (type == WS_EVT_DATA)
      {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
          enqueue(client->id(), false, data, len);
      }
    }

    WiFiEventHandler onJoin, onLeave;

    void logRequest(AsyncWebServerRequest *request, const char *what)
    {
      Serial.printf("HTTP %s%s from %s -> %s\n", request->host().c_str(), request->url().c_str(),
                    request->client()->remoteIP().toString().c_str(), what);
    }

    // Any other host/path: redirect to the app, so phones show it as captive portal.
    void redirectToApp(AsyncWebServerRequest *request)
    {
      logRequest(request, "redirect");
      request->redirect(String("http://") + WiFi.softAPIP().toString() + "/");
    }

    void handleRoot(AsyncWebServerRequest *request)
    {
      if (request->host() != WiFi.softAPIP().toString())
      {
        redirectToApp(request);
        return;
      }
      logRequest(request, "app");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (const uint8_t *)INDEX_HTML, strlen_P(INDEX_HTML));
      response->addHeader("Cache-Control", "no-store"); // phones must pick up a new firmware's app at once
      request->send(response);
    }

    // CSV export with a proper file name: browsers like DuckDuckGo name blob
    // downloads after a random id, but honour Content-Disposition. The app
    // uploads the CSV (streamed to flash, so size is no RAM problem) and then
    // downloads it from /csv.
    constexpr const char *CSV_PATH = "/export.csv";
    constexpr size_t CSV_MAX = 256 * 1024;
    File csvFile;
    String csvName = "ButtonSpiel.csv";
    bool fsReady = false;

    void onCsvBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    {
      if (index == 0)
      {
        if (!fsReady)
          return;
        LittleFS.remove(CSV_PATH); // never hand out an older export by mistake
        if (total > CSV_MAX)
          return;
        String name = request->header("X-File");
        name.replace("\"", "");
        csvName = name.length() ? name : String("ButtonSpiel.csv");
        csvFile = LittleFS.open(CSV_PATH, "w");
      }
      if (csvFile)
        csvFile.write(data, len);
      if (csvFile && index + len == total)
        csvFile.close();
    }

    void onCsvUploaded(AsyncWebServerRequest *request)
    {
      bool ok = fsReady && request->contentLength() <= CSV_MAX && LittleFS.exists(CSV_PATH);
      Serial.printf("CSV upload %u bytes as %s: %s\n", (unsigned)request->contentLength(), csvName.c_str(), ok ? "ok" : "failed");
      request->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "failed");
    }

    void onCsvDownload(AsyncWebServerRequest *request)
    {
      if (!fsReady || !LittleFS.exists(CSV_PATH))
      {
        request->send(404, "text/plain", "no export");
        return;
      }
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, CSV_PATH, "text/csv; charset=utf-8");
      response->addHeader("Content-Disposition", "attachment; filename=\"" + csvName + "\"");
      response->addHeader("Cache-Control", "no-store");
      request->send(response);
    }
  }

  void begin()
  {
    fsReady = LittleFS.begin(); // formats an empty flash area on first use
    if (!fsReady)
      Serial.println("LittleFS not available, CSV download falls back to the browser");
    onJoin = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected &e)
                                               { Serial.printf("Phone joined: %s\n", macToString(e.mac).c_str()); });
    onLeave = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected &e)
                                                   { Serial.printf("Phone left: %s\n", macToString(e.mac).c_str()); });
    dns.start(53, "*", WiFi.softAPIP());
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/csv", HTTP_POST, onCsvUploaded, nullptr, onCsvBody);
    server.on("/csv", HTTP_GET, onCsvDownload);
    server.onNotFound(redirectToApp);
    server.begin();
    started = true;
    Serial.printf("Web app: connect to WiFi \"%s\", open http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }

  void update()
  {
    if (!started)
      return;
    dns.processNextRequest();
    while (tail != head)
    {
      Pending &p = queue[tail];
      if (p.greet)
      {
        ws.text(p.client, gateway::stateJson());
        ws.text(p.client, gateway::resultsJson()); // the app stores the ones it has not seen yet
      }
      else
        runCommand(p.client, p.text);
      tail = (tail + 1) % QUEUE_SIZE;
    }
    static uint32_t lastCleanup = 0;
    if (millis() - lastCleanup > 1000)
    {
      lastCleanup = millis();
      ws.cleanupClients(); // drops clients that went away
    }
  }

  void broadcast(const String &json)
  {
    if (!started)
      return;
    ws.textAll(json); // queued, never blocks
  }
}
