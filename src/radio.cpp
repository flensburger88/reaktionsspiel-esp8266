#include "radio.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

namespace radio
{
  namespace
  {
    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t ownMac[6];

    // The ESP8266 calls the receive callback from the SDK task, never
    // concurrently with loop(), so a plain ring buffer is safe here.
    constexpr uint8_t QUEUE_SIZE = 16;
    Msg queue[QUEUE_SIZE];
    uint8_t head = 0;
    uint8_t tail = 0;

    void onReceive(uint8_t *mac, uint8_t *data, uint8_t len)
    {
      if (len != sizeof(Msg))
        return;
      const Msg *m = reinterpret_cast<const Msg *>(data);
      if (m->magic != MSG_MAGIC || m->version != MSG_VERSION)
        return;
      uint8_t next = (head + 1) % QUEUE_SIZE;
      if (next == tail)
        return; // full: drop, the state is re-sent anyway
      memcpy(&queue[head], data, sizeof(Msg));
      head = next;
    }
  }

  bool begin(bool gateway)
  {
    esp_now_deinit();

    // Never join a router: a stored network (e.g. from the V2 WiFiManager)
    // would move the radio to another channel and break ESP-NOW.
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    wifi_station_set_auto_connect(false);
    if (gateway)
    {
      WiFi.mode(WIFI_AP_STA);
      WiFi.disconnect();
      WiFi.softAP(AP_SSID, AP_PASSWORD, ESPNOW_CHANNEL);
    }
    else
    {
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      wifi_set_channel(ESPNOW_CHANNEL);
    }
    WiFi.macAddress(ownMac);

    if (esp_now_init() != 0)
      return false;
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(onReceive);
    esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, nullptr, 0);
    return true;
  }

  void send(Msg &m)
  {
    m.magic = MSG_MAGIC;
    m.version = MSG_VERSION;
    memcpy(m.from, ownMac, 6);
    esp_now_send(broadcastMac, reinterpret_cast<uint8_t *>(&m), sizeof(Msg));
  }

  bool receive(Msg &out)
  {
    if (tail == head)
      return false;
    out = queue[tail];
    tail = (tail + 1) % QUEUE_SIZE;
    return true;
  }

  const uint8_t *selfMac()
  {
    return ownMac;
  }
}
