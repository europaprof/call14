#pragma once

#include <Arduino.h>

// JUZIPi/SmallTV ESP8266 display connections.
constexpr uint8_t PIN_TFT_SCLK = 14;  // D5
constexpr uint8_t PIN_TFT_MOSI = 13;  // D7
constexpr uint8_t PIN_TFT_CS = 15;    // D8, held low for normal ESP8266 boot
constexpr uint8_t PIN_TFT_DC = 0;     // boot strap pin: display only
constexpr uint8_t PIN_TFT_RST = 2;    // boot strap pin: display only
constexpr uint8_t PIN_TFT_BACKLIGHT = 5;  // active LOW

// Verified by continuity test: rear pad "O" connects to GPIO4.
constexpr uint8_t PIN_CALL_BUTTON = 4;
