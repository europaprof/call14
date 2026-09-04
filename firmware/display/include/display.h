#pragma once

#include <Arduino.h>

namespace display {
void begin();
void showDiagnostic(const char* networkState, const char* address);
void beginUi();
void loop();
void setElevatorState(const String& state, int floor, const String& direction,
                      int etaSeconds);
void showLiftCall();
void showMotion(const String& location);
void setHomeState(const String& condition, float temperature, int humidity,
                  float wind, int pm25);
void setInverterState(bool mainsAvailable, float loadWatts, float energyToday,
                      float outputVoltage, float outputCurrent,
                      bool chargerOn, float chargerWatts);
void setChargerState(bool chargerOn, float powerWatts, float energyKwh,
                     float voltage, float currentAmps);
void setAlarm(bool active);
void setHaConnected(bool connected);
bool elevatorVisible();
void nextScreen();
void cycleBrightness();
void setBrightness(uint8_t percent);
uint8_t brightness();
bool setViewMode(const String& mode);
const char* viewModeName();
const char* activeScreenName();
}  // namespace display
