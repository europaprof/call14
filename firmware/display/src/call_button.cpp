#include "call_button.h"

#include <Arduino.h>

#include "display.h"
#include "home_assistant.h"
#include "pins.h"

namespace {
bool stableState = HIGH;
bool sampledState = HIGH;
uint32_t changedAt = 0;
uint32_t pressedAt = 0;
uint32_t lastRelease = 0;
uint8_t clickCount = 0;
}

namespace callButton {
void begin() {
  pinMode(PIN_CALL_BUTTON, INPUT_PULLUP);
  stableState = sampledState = digitalRead(PIN_CALL_BUTTON);
}

void loop() {
  const bool sample = digitalRead(PIN_CALL_BUTTON);
  if (sample != sampledState) {
    sampledState = sample;
    changedAt = millis();
  }
  if (sample != stableState && millis() - changedAt >= 35) {
    stableState = sample;
    if (stableState == LOW) {
      pressedAt = millis();
    } else {
      const uint32_t held = millis() - pressedAt;
      if (held >= 900) {
        clickCount = 0;
        display::cycleBrightness();
      } else {
        ++clickCount;
        lastRelease = millis();
      }
    }
  }
  if (clickCount && millis() - lastRelease > 350) {
    if (clickCount == 1) {
      if (display::elevatorVisible()) display::nextScreen();
      else homeAssistant::callLift();
    }
    else display::nextScreen();
    clickCount = 0;
  }
}
}  // namespace callButton
