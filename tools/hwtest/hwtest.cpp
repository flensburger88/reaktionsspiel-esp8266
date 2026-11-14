// Hardware test for one soldered button: LEDs and push button, nothing else.
//
//   pio run -e hwtest -t upload -t monitor
//
// LEDs: cycle red, green, blue, white, then one LED at a time (shows a broken
//       link in the chain: everything after it stays dark).
// Button: while pressed all LEDs flash white; every press/release is logged
//         on the serial monitor (115200 baud) with a counter.
#include <Arduino.h>
#include <FastLED.h>

#ifndef TEST_LED_PIN
#define TEST_LED_PIN D1
#endif
#ifndef TEST_BUTTON_PIN
#define TEST_BUTTON_PIN D3
#endif
#ifndef NUM_LEDS
#define NUM_LEDS 4 // finished buttons have 4 LEDs
#endif
#define BRIGHTNESS 60 // low: USB power is enough

CRGB leds[NUM_LEDS];
const CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White};
const char *names[] = {"rot", "gruen", "blau", "weiss"};

void setup() {
  Serial.begin(115200);
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  FastLED.addLeds<WS2812B, TEST_LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  delay(300);
  Serial.printf("\nButtonSpiel Hardwaretest: LEDs an GPIO%d, Taster an GPIO%d\n", TEST_LED_PIN, TEST_BUTTON_PIN);
}

void loop() {
  static bool wasDown = false;
  static uint32_t presses = 0, downSince = 0, lastStep = 0;
  static uint8_t step = 0;
  const uint8_t steps = 4 * 2 + NUM_LEDS; // 4 colors on/off, then a running dot

  bool down = digitalRead(TEST_BUTTON_PIN) == LOW;
  if (down != wasDown) {
    delay(15); // debounce
    down = digitalRead(TEST_BUTTON_PIN) == LOW;
  }
  if (down && !wasDown) {
    downSince = millis();
    Serial.printf("Taster GEDRUECKT  (#%lu)\n", (unsigned long)++presses);
  } else if (!down && wasDown) {
    Serial.printf("Taster losgelassen nach %lu ms\n", (unsigned long)(millis() - downSince));
  }
  wasDown = down;
  static uint32_t lastLevel = 0;
  if (millis() - lastLevel >= 500) { // raw level: HIGH = open, LOW = pressed
    lastLevel = millis();
    Serial.printf("D3 = %s\n", digitalRead(TEST_BUTTON_PIN) ? "HIGH (offen)" : "LOW (gedrueckt)");
  }

  if (down) {
    fill_solid(leds, NUM_LEDS, (millis() / 60) % 2 ? CRGB::White : CRGB::Black); // fast flash
  } else if (millis() - lastStep >= 400) {
    lastStep = millis();
    step = (step + 1) % steps;
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    if (step < 8) {
      if (step % 2 == 0) {
        fill_solid(leds, NUM_LEDS, colors[step / 2]);
        Serial.printf("alle LEDs %s\n", names[step / 2]);
      }
    } else {
      leds[step - 8] = CRGB::Yellow;
      Serial.printf("nur LED %d\n", step - 8 + 1);
    }
  }
  FastLED.show();
  delay(5);
}
