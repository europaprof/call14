#pragma once

#include <Arduino.h>

namespace network {
void begin();
void loop();
bool stationConnected();
const char* modeName();
String address();
}  // namespace network
