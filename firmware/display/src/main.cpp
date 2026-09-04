#include <Arduino.h>

#include "call_button.h"
#include "display.h"
#include "home_assistant.h"
#include "network.h"
#include "pins.h"
#include "web_portal.h"

namespace {
bool displaySafeMode = false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nSD_PRO lift display diagnostic firmware");

#if defined(RECOVERY_BUILD) || defined(NETWORK_SAFE_BUILD)
#ifdef RECOVERY_BUILD
  Serial.println("RECOVERY_BUILD: network and OTA only");
#else
  Serial.println("NETWORK_SAFE_BUILD: static IP, OTA, display disabled");
#endif
  network::begin();
  webPortal::begin();
#else
  // Keep the active-low backlight dark until normal display startup is chosen.
  pinMode(PIN_TFT_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_TFT_BACKLIGHT, HIGH);
  pinMode(PIN_CALL_BUTTON, INPUT_PULLUP);
  uint8_t lowSamples = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    if (digitalRead(PIN_CALL_BUTTON) == LOW) ++lowSamples;
    delay(15);
  }
  displaySafeMode = lowSamples >= 7;
  if (displaySafeMode) {
    Serial.println("DISPLAY SAFE MODE: GPIO4 held LOW; display disabled");
  } else {
    display::begin();
    display::showDiagnostic("starting", "please wait");
  }
  network::begin();
  webPortal::begin();
#if defined(ENABLE_MQTT)
  if (!displaySafeMode) homeAssistant::begin();
#endif
  if (!displaySafeMode) {
    display::showDiagnostic(network::modeName(), network::address().c_str());
    delay(1400);
    callButton::begin();
    display::beginUi();
  }
#endif
}

void loop() {
  network::loop();
  webPortal::loop();
#if defined(ENABLE_MQTT) && !defined(RECOVERY_BUILD) && !defined(NETWORK_SAFE_BUILD)
  if (!displaySafeMode) homeAssistant::loop();
#endif
#if !defined(RECOVERY_BUILD) && !defined(NETWORK_SAFE_BUILD)
  if (!displaySafeMode) {
    callButton::loop();
    display::loop();
  }
#endif
  delay(2);
}
