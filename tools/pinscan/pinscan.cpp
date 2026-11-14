// Wiring finder: sends LED data on one pin after the other (4 s each) and logs
// button presses on every pin that is not sending at the moment.
//
//   pio run -e pinscan -t upload     then serial monitor at 115200 baud
//
// Each pin sends its own colour (D1 red, D2 green, D3 blue, D4 white, D5 yellow,
// D6 cyan, D7 magenta), so the colour you see tells which pin the LEDs are on. A press shows up as "Taster an Dx".
#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

struct ScanPin { uint8_t gpio; const char *name; CLEDController *ctl; bool lastDown; uint16_t presses; };
ScanPin pins[] = {{5, "D1"}, {4, "D2"}, {0, "D3"}, {2, "D4"}, {14, "D5"}, {12, "D6"}, {13, "D7"}};
const int N = sizeof(pins) / sizeof(pins[0]);
// one fixed colour per pin, so "the LEDs were blue" tells which pin drives them
const CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White, CRGB::Yellow, CRGB::Cyan, CRGB::Magenta};
const char *colorNames[] = {"ROT", "GRUEN", "BLAU", "WEISS", "GELB", "TUERKIS", "LILA"};

void setup() {
  Serial.begin(115200);
  pins[0].ctl = &FastLED.addLeds<WS2812B, 5, GRB>(leds, NUM_LEDS);
  pins[1].ctl = &FastLED.addLeds<WS2812B, 4, GRB>(leds, NUM_LEDS);
  pins[2].ctl = &FastLED.addLeds<WS2812B, 0, GRB>(leds, NUM_LEDS);
  pins[3].ctl = &FastLED.addLeds<WS2812B, 2, GRB>(leds, NUM_LEDS);
  pins[4].ctl = &FastLED.addLeds<WS2812B, 14, GRB>(leds, NUM_LEDS);
  pins[5].ctl = &FastLED.addLeds<WS2812B, 12, GRB>(leds, NUM_LEDS);
  pins[6].ctl = &FastLED.addLeds<WS2812B, 13, GRB>(leds, NUM_LEDS);
  for (auto &p : pins) { pinMode(p.gpio, INPUT_PULLUP); p.lastDown = false; p.presses = 0; }
  delay(300);
  Serial.println("\nButtonSpiel Pin-Suche gestartet");
}

void loop() {
  static int active = -1;
  static uint32_t since = 0;
  if (active < 0 || millis() - since >= 4000) {
    if (active >= 0) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      pinMode(pins[active].gpio, OUTPUT);
      pins[active].ctl->showLeds(60); // switch the old chain off again
      pinMode(pins[active].gpio, INPUT_PULLUP);
    }
    active = (active + 1) % N;
    since = millis();
    Serial.printf("LED-Daten jetzt auf %s (GPIO%d) -> %s\n", pins[active].name, pins[active].gpio, colorNames[active]);
    if (active == 0) {
      Serial.print("Tastendruecke bisher:");
      for (auto &p : pins) Serial.printf(" %s=%u", p.name, p.presses);
      Serial.println();
    }
  }
  fill_solid(leds, NUM_LEDS, colors[active]);
  pinMode(pins[active].gpio, OUTPUT);
  pins[active].ctl->showLeds(60);

  for (int i = 0; i < N; i++) {
    if (i == active) continue;
    bool down = digitalRead(pins[i].gpio) == LOW;
    if (down && !pins[i].lastDown) pins[i].presses++;
    if (down != pins[i].lastDown)
      Serial.printf("Taster an %s %s\n", pins[i].name, down ? "GEDRUECKT" : "losgelassen");
    pins[i].lastDown = down;
  }
  delay(20);
}
