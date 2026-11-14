#pragma once
#include <Arduino.h>

// Hardware (same wiring as ButtonSpielV2)
#define LED_PIN D1
#define NUM_LEDS 8
#define BUTTON_PIN D3 // GPIO0, active LOW
#define LED_BRIGHTNESS 120

// Radio / WiFi
#define ESPNOW_CHANNEL 1
#define AP_SSID "ButtonSpiel"
#define AP_PASSWORD "" // open network; use >= 8 chars to protect it

// Timing (ms)
#define ELECTION_MIN_MS 1500 // listen this long (+ random) for a gateway before becoming one
#define ELECTION_RANDOM_MS 1500
#define STATE_INTERVAL_MS 100 // gateway re-broadcasts the game state
#define HELLO_INTERVAL_MS 1000 // units announce themselves
#define UNIT_OFFLINE_MS 3500
#define GATEWAY_LOST_MS 3000
#define GATEWAY_REELECT_MS 15000 // unit restarts and may become the gateway itself
#define PRESS_RESEND_MS 40
#define PRESS_RESEND_MAX_MS 3000
#define DEBOUNCE_MS 30
#define COUNTDOWN_MS 3000
#define PAUSE_MIN_MS 300
#define PAUSE_MAX_MS 1200
#define DUEL_PAUSE_MS 150 // duel: next button right after a hit, just long enough to see the green flash
#define RESULT_SHOW_MS 4000
#define IDENTIFY_MS 2000
#define FALSE_PRESS_PENALTY_MS 500

// Game defaults / limits
#define DEFAULT_PRESSES 10
#define MAX_PRESSES 50
#define DEFAULT_TIMEOUT_MS 5000
#define MIN_TIMEOUT_MS 1000
#define MAX_TIMEOUT_MS 30000
#define MAX_UNITS 10
#define MAX_RESULTS 10 // finished games kept on the gateway until the phone has them
#define MAX_COMP_ID 24
#define MAX_PLAYER_NAME 16
