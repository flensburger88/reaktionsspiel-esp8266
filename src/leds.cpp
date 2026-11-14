#include "leds.h"
#include "config.h"

namespace leds
{
  namespace
  {
    CRGB pixels[NUM_LEDS];
    Pattern pattern = OFF;
    uint32_t patternSince = 0;
    CRGB flashColor;
    uint32_t flashUntil = 0;
    uint32_t lastRender = 0;

    void fill(CRGB c)
    {
      fill_solid(pixels, NUM_LEDS, c);
    }

    // The onboard LED (GPIO2, active LOW) mirrors the ring, so a bare
    // NodeMCU is usable for testing.
    void show()
    {
      bool lit = false;
      for (int i = 0; i < NUM_LEDS && !lit; i++)
        lit = max(pixels[i].r, max(pixels[i].g, pixels[i].b)) > 100; // dim idle breathing stays off
      digitalWrite(LED_BUILTIN, lit ? LOW : HIGH);
      FastLED.show();
    }

    bool blinkOn(uint32_t now, uint32_t periodMs, uint32_t onMs)
    {
      return (now - patternSince) % periodMs < onMs;
    }

    void render()
    {
      uint32_t now = millis();
      lastRender = now;
      if ((int32_t)(flashUntil - now) > 0)
      {
        fill(flashColor);
        show();
        return;
      }
      switch (pattern)
      {
      case OFF:
        fill(CRGB::Black);
        break;
      case ELECTION:
        fill(CRGB(20, 20, 20));
        break;
      case NO_GATEWAY:
        fill(blinkOn(now, 1000, 150) ? CRGB(120, 90, 0) : CRGB::Black);
        break;
      case IDLE_UNIT:
        fill(CRGB(0, 0, beatsin8(12, 5, 60)));
        break;
      case IDLE_GATEWAY:
      {
        uint8_t v = beatsin8(12, 5, 60);
        fill(CRGB(0, v, v));
        break;
      }
      case COUNTDOWN:
        fill(blinkOn(now, 1000, 250) ? CRGB::White : CRGB::Black);
        break;
      case TARGET_A:
      case TARGET_B:
      {
        // same chase as V2: color, color, black, black rotating every 100 ms
        CRGB color = pattern == TARGET_A ? CRGB::Red : CRGB::Blue;
        uint32_t step = (now - patternSince) / 100;
        for (int i = 0; i < NUM_LEDS; i++)
          pixels[i] = ((i + step) % 4 < 2) ? color : CRGB::Black;
        break;
      }
      case RESULT:
      {
        uint8_t v = beatsin8(40, 20, 200);
        fill(CRGB(v, 0, v));
        break;
      }
      case RESULT_BEST:
        fill_rainbow(pixels, NUM_LEDS, (now / 10) & 0xFF, 256 / NUM_LEDS);
        break;
      case RESULT_A:
        fill(CRGB(beatsin8(40, 20, 255), 0, 0));
        break;
      case RESULT_B:
        fill(CRGB(0, 0, beatsin8(40, 20, 255)));
        break;
      case IDENTIFY:
        fill(blinkOn(now, 300, 150) ? CRGB::Blue : CRGB::Black);
        break;
      case TEST:
      {
        // 400 ms steps: 4 colours with a dark step in between, then a running dot,
        // so a dead LED (and everything after it in the chain) is easy to spot
        static const CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White};
        uint32_t step = (now - patternSince) / 400 % (8 + NUM_LEDS);
        fill(CRGB::Black);
        if (step < 8)
        {
          if (step % 2 == 0)
            fill(colors[step / 2]);
        }
        else
          pixels[step - 8] = CRGB(255, 160, 0);
        break;
      }
      }
      show();
    }
  }

  void begin()
  {
    pinMode(LED_BUILTIN, OUTPUT);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(pixels, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    render();
  }

  void setPattern(Pattern p)
  {
    if (p == pattern)
      return;
    pattern = p;
    patternSince = millis();
    render();
  }

  void flash(CRGB color, uint16_t ms)
  {
    flashColor = color;
    flashUntil = millis() + ms;
    render();
  }

  void update()
  {
    if (millis() - lastRender >= 20)
      render();
  }
}
