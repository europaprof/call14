#include "display.h"

#include <TFT_eSPI.h>
#include <time.h>

#include "pins.h"

namespace {
TFT_eSPI* tft = nullptr;
TFT_eSprite* liftAnimation = nullptr;
enum class Screen { Boot, Clock, Elevator, EnergyFlow, Inverter, Charger, Motion, Alarm, AllClear };
enum class ViewMode { Auto, Home, Flow, Inverter, Charger, Lift };
Screen screen = Screen::Boot;
ViewMode viewMode = ViewMode::Auto;
uint32_t screenStarted = 0;
uint32_t lastFrame = 0;
uint32_t lastLiftFrame = 0;
uint32_t lastWeatherFrame = 0;
uint8_t weatherPhase = 0;
int lastClockMinute = -1;
int lastMetricPage = -1;
bool screenDirty = true;
bool weatherDirty = true;
bool elevatorDirty = true;
bool inverterDirty = true;
bool chargerDirty = true;
bool flowDirty = true;
uint32_t lastFlowFrame = 0;
uint8_t flowPhase = 0;
uint32_t weatherChangedAt = 0;
String liftState = "idle";
String liftDirection = "down";
String actualLiftState = "idle";
String actualLiftDirection = "none";
int liftFloor = 14;
int liftEta = 0;
bool liftCallActive = false;
uint32_t liftCallStarted = 0;
String motionLocation = "Floor 14";
String weatherCondition = "unknown";
float outsideTemperature = NAN;
float windSpeed = 0;
int outsideHumidity = 0;
int pm25 = -1;
bool mainsAvailable = true;
float inverterLoadWatts = NAN;
float inverterEnergyToday = NAN;
float inverterOutputVoltage = NAN;
float inverterOutputCurrent = NAN;
bool chargerOn = false;
float chargerWatts = NAN;
float chargerEnergyToday = NAN;
float chargerVoltage = NAN;
float chargerCurrent = NAN;
bool alarmActive = false;
bool haConnected = false;
bool statusDirty = true;
uint8_t brightnessPercent = 70;

constexpr uint16_t NAVY = 0x0863;
constexpr uint16_t CARD = 0x18E3;
constexpr uint16_t BLUE = 0x34BF;
constexpr uint16_t GREEN = 0x4E69;
constexpr uint16_t SOFT_WHITE = 0xE73C;
constexpr uint16_t MUTED = 0x8410;
constexpr uint16_t ORANGE = 0xFD20;
constexpr uint16_t ICE_BLUE = 0x5D9F;

Screen selectedScreen() {
  switch (viewMode) {
    case ViewMode::Home: return Screen::Clock;
    case ViewMode::Flow: return Screen::EnergyFlow;
    case ViewMode::Inverter: return Screen::Inverter;
    case ViewMode::Charger: return Screen::Charger;
    case ViewMode::Lift: return Screen::Elevator;
    case ViewMode::Auto: return mainsAvailable ? Screen::Clock : Screen::Inverter;
  }
  return Screen::Clock;
}

void returnToSelectedScreen() {
  screen = selectedScreen();
  screenStarted = millis();
  screenDirty = true;
  lastClockMinute = -1;
}

void centered(const String& text, int y, uint8_t size, uint16_t color) {
  tft->setTextSize(size);
  tft->setTextColor(color);
  tft->setCursor((240 - tft->textWidth(text)) / 2, y);
  tft->print(text);
}

void applyBrightness() {
  const uint16_t pwm = 1023 - (brightnessPercent * 1023UL / 100);
  analogWrite(PIN_TFT_BACKLIGHT, pwm);
}

void drawBoot() {
  const uint32_t elapsed = millis() - screenStarted;
  if (screenDirty) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
    tft->fillCircle(120, 91, 36, BLUE);
    tft->fillCircle(120, 91, 28, TFT_BLACK);
    tft->fillRoundRect(38, 145, 164, 5, 3, BLUE);
    centered("LIFT DISPLAY", 171, 2, SOFT_WHITE);
  }
  if (elapsed > 1800) {
    screen = Screen::Clock;
    screenStarted = millis();
    screenDirty = true;
    lastClockMinute = -1;
    return;
  }
}

void drawWeatherIcon() {
  // Render off-screen, then copy in one SPI transaction: animation without flash.
  liftAnimation->fillSprite(TFT_BLACK);
  const bool rainy = weatherCondition.indexOf("rain") >= 0;
  const bool snowy = weatherCondition.indexOf("snow") >= 0;
  const bool foggy = weatherCondition.indexOf("fog") >= 0;
  const bool cloudy = weatherCondition.indexOf("cloud") >= 0;
  const bool partly = weatherCondition.indexOf("partly") >= 0;
  const int bob = cloudy || rainy || snowy ? (weatherPhase < 4 ? weatherPhase : 7 - weatherPhase) : 0;

  if (foggy) {
    const int shift = weatherPhase % 4;
    for (int y = 17; y <= 53; y += 12)
      liftAnimation->drawRoundRect(10 + ((y / 12 + shift) % 3), y, 72, 5, 3, MUTED);
    liftAnimation->pushSprite(10, 130);
    return;
  }
  if (!rainy && !snowy && !cloudy) {
    liftAnimation->fillCircle(47, 34, 22, TFT_YELLOW);
    const int angleOffset = weatherPhase * 6;
    for (int a = 0; a < 360; a += 45) {
      const float r = (a + angleOffset) * DEG_TO_RAD;
      liftAnimation->drawLine(47 + 29 * cosf(r), 34 + 29 * sinf(r),
                              47 + 39 * cosf(r), 34 + 39 * sinf(r), TFT_YELLOW);
    }
    liftAnimation->pushSprite(10, 130);
    return;
  }

  if (partly) liftAnimation->fillCircle(25, 18 + bob, 15, TFT_YELLOW);
  const int cloudY = bob;
  liftAnimation->fillCircle(33, 32 + cloudY, 17, MUTED);
  liftAnimation->fillCircle(56, 25 + cloudY, 23, MUTED);
  liftAnimation->fillRoundRect(21, 32 + cloudY, 65, 22, 10, MUTED);
  if (rainy) {
    for (int i = 0; i < 3; ++i) {
      const int x = 31 + i * 21;
      const int drop = (weatherPhase * 3 + i * 6) % 15;
      liftAnimation->drawLine(x, 57 + drop, x - 4, 66 + drop, TFT_CYAN);
    }
  } else if (snowy) {
    for (int i = 0; i < 3; ++i) {
      const int x = 30 + i * 22;
      const int y = 60 + ((weatherPhase * 2 + i * 5) % 13);
      liftAnimation->drawFastHLine(x - 3, y, 7, TFT_WHITE);
      liftAnimation->drawFastVLine(x, y - 3, 7, TFT_WHITE);
    }
  }
  liftAnimation->pushSprite(10, 130);
}

void drawClock() {
  const time_t now = time(nullptr);
  struct tm local {};
  localtime_r(&now, &local);
  const int minuteKey = local.tm_hour * 60 + local.tm_min;
  const bool fullRedraw = screenDirty;
  const bool redrawWeather = fullRedraw ||
      (weatherDirty && millis() - weatherChangedAt >= 180);
  const bool redrawStatus = fullRedraw || statusDirty;
  const bool animateWeather = millis() - lastWeatherFrame >= 180;
  if (!fullRedraw && !redrawWeather && !redrawStatus && !animateWeather &&
      minuteKey == lastClockMinute) return;

  if (fullRedraw) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
  }
  if (redrawWeather) {
    // Only the weather panel is cleared. The clock stays continuously visible.
    tft->fillRect(0, 132, 240, 108, TFT_BLACK);
    drawWeatherIcon();
    uint16_t temperatureColor = SOFT_WHITE;
    if (!isnan(outsideTemperature) && outsideTemperature <= 5) temperatureColor = ICE_BLUE;
    else if (!isnan(outsideTemperature) && outsideTemperature >= 27) temperatureColor = ORANGE;
    tft->setTextColor(temperatureColor, TFT_BLACK);
    tft->setTextSize(5);
    tft->setCursor(111, 145);
    if (isnan(outsideTemperature)) tft->print("--");
    else tft->printf("%.0f", outsideTemperature);
    tft->setTextSize(3);
    tft->print("o");
    // Stable, simultaneous essentials; no rotating text or distracting motion.
    tft->setTextSize(2);
    tft->setTextColor(SOFT_WHITE, TFT_BLACK);
    tft->setCursor(18, 215);
    tft->printf("HUM %d%%", outsideHumidity);
    tft->setCursor(130, 215);
    tft->printf("PM %d", max(0, pm25));
    if (windSpeed >= 20) {
      tft->setTextSize(1);
      tft->setTextColor(ORANGE, TFT_BLACK);
      tft->setCursor(112, 187);
      tft->printf("WIND %.0f KM/H", windSpeed);
    }
    weatherDirty = false;
  }
  if (animateWeather && !redrawWeather) {
    lastWeatherFrame = millis();
    weatherPhase = (weatherPhase + 1) & 7;
    drawWeatherIcon();
  } else if (redrawWeather) {
    lastWeatherFrame = millis();
  }
  if (redrawStatus) {
    tft->fillRect(0, 0, 240, 29, TFT_BLACK);
    tft->setTextSize(1);
    tft->setTextColor(MUTED, TFT_BLACK);
    tft->setCursor(9, 11);
    tft->print("HA");
    tft->fillCircle(31, 14, 4, haConnected ? GREEN : ORANGE);
    tft->setTextSize(2);
    const bool liftMoving = actualLiftState == "moving" ||
                            actualLiftDirection == "up" ||
                            actualLiftDirection == "down";
    tft->setTextColor(liftMoving ? ORANGE : SOFT_WHITE, TFT_BLACK);
    tft->setCursor(69, 6);
    tft->printf("LIFT %d", liftFloor);
    if (actualLiftDirection == "up")
      tft->fillTriangle(169, 19, 181, 19, 175, 7, ORANGE);
    else if (actualLiftDirection == "down")
      tft->fillTriangle(169, 8, 181, 8, 175, 20, ORANGE);
    if (alarmActive) {
      tft->fillCircle(220, 14, 10, TFT_RED);
      tft->setTextSize(2);
      tft->setTextColor(TFT_WHITE, TFT_RED);
      tft->setCursor(217, 7);
      tft->print("!");
    }
    statusDirty = false;
  }
  if (fullRedraw || minuteKey != lastClockMinute) {
    lastClockMinute = minuteKey;
    tft->fillRect(0, 30, 240, 94, TFT_BLACK);

    char timeText[6];
    if (now > 100000) strftime(timeText, sizeof(timeText), "%H:%M", &local);
    else snprintf(timeText, sizeof(timeText), "--:--");
    centered(timeText, 38, 6, SOFT_WHITE);

    char dateText[24];
    if (now > 100000) strftime(dateText, sizeof(dateText), "%a, %d %b", &local);
    else snprintf(dateText, sizeof(dateText), "Syncing time");
    centered(dateText, 100, 2, MUTED);
  }

}

void drawAlarm(bool clear) {
  if (millis() - screenStarted > 6000) {
    returnToSelectedScreen();
    lastMetricPage = -1;
    return;
  }
  if (!screenDirty) return;
  screenDirty = false;
  tft->fillScreen(TFT_BLACK);
  const uint16_t color = clear ? GREEN : TFT_RED;
  tft->fillCircle(120, 91, 57, color);
  tft->fillCircle(120, 91, 41, TFT_BLACK);
  centered(clear ? "ALL CLEAR" : "ALARM", 166, 3, color);
}

void drawFlowLine(int x1, int x2, int y, bool active, uint16_t color) {
  tft->fillRect(x1, y - 5, x2 - x1 + 1, 11, TFT_BLACK);
  tft->drawFastHLine(x1, y, x2 - x1 + 1, active ? color : CARD);
  if (!active) return;
  for (int x = x1 + (flowPhase % 12); x <= x2; x += 12)
    tft->fillCircle(x, y, 3, color);
}

void drawEnergyFlow() {
  if (screenDirty) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
    centered("ENERGY FLOW", 10, 2, SOFT_WHITE);

    // Grid tower.
    tft->drawFastVLine(29, 48, 48, MUTED);
    tft->drawLine(15, 60, 43, 60, MUTED);
    tft->drawLine(19, 74, 39, 74, MUTED);
    tft->drawLine(29, 48, 15, 96, MUTED);
    tft->drawLine(29, 48, 43, 96, MUTED);

    // Battery / charger.
    tft->drawRoundRect(96, 51, 49, 42, 5, ORANGE);
    tft->fillRect(145, 63, 5, 16, ORANGE);
    tft->drawFastVLine(108, 60, 23, ORANGE);
    tft->drawFastHLine(103, 71, 11, ORANGE);
    tft->drawFastHLine(127, 71, 12, ORANGE);

    // Home.
    tft->drawTriangle(194, 69, 212, 50, 230, 69, SOFT_WHITE);
    tft->drawRect(198, 69, 28, 27, SOFT_WHITE);
    tft->drawRect(208, 78, 8, 18, SOFT_WHITE);
    flowDirty = true;
  }

  const bool chargingFlow = mainsAvailable && chargerOn &&
                            !isnan(chargerWatts) && chargerWatts > 5;
  const bool inverterFlow = !mainsAvailable && !isnan(inverterLoadWatts) &&
                            inverterLoadWatts > 5;
  if (millis() - lastFlowFrame >= 150) {
    lastFlowFrame = millis();
    flowPhase = (flowPhase + 3) % 12;
    drawFlowLine(47, 92, 72, chargingFlow, GREEN);
    drawFlowLine(153, 190, 72, inverterFlow, ORANGE);
  }

  if (!flowDirty) return;
  flowDirty = false;
  tft->fillRect(5, 108, 230, 127, TFT_BLACK);
  tft->setTextSize(2);
  tft->setTextColor(MUTED, TFT_BLACK);
  tft->setCursor(8, 112); tft->print("GRID");
  tft->setCursor(178, 112); tft->print("HOME");

  tft->setTextColor(chargingFlow ? GREEN : SOFT_WHITE, TFT_BLACK);
  tft->setCursor(8, 136);
  if (isnan(chargerWatts)) tft->print("-- W");
  else tft->printf("%.0f W", chargerWatts);

  tft->setTextColor(inverterFlow ? ORANGE : MUTED, TFT_BLACK);
  tft->setCursor(174, 136);
  if (isnan(inverterLoadWatts)) tft->print("-- W");
  else tft->printf("%.0f W", inverterLoadWatts);

  centered(mainsAvailable ? (chargingFlow ? "CHARGING" : "GRID ON") : "ON BATTERY",
           165, 2, mainsAvailable ? GREEN : ORANGE);

  String charged = "CHARGED ";
  charged += isnan(chargerEnergyToday) ? "--" : String(chargerEnergyToday, 2);
  charged += " kWh";
  centered(charged, 194, 2, GREEN);
  String used = "USED ";
  used += isnan(inverterEnergyToday) ? "--" : String(inverterEnergyToday, 2);
  used += " kWh";
  centered(used, 218, 2, ORANGE);
}

void drawInverter() {
  if (screenDirty) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
    tft->fillRoundRect(12, 16, 7, 208, 4, ORANGE);
    centered("POWER OUTAGE", 14, 2, ORANGE);
    inverterDirty = true;
  }
  if (!inverterDirty) return;
  inverterDirty = false;

  tft->fillRect(25, 43, 210, 190, TFT_BLACK);
  tft->setTextColor(SOFT_WHITE, TFT_BLACK);
  if (isnan(inverterLoadWatts)) {
    centered("-- W", 54, 6, SOFT_WHITE);
  } else {
    const String load = String(inverterLoadWatts, 0) + " W";
    centered(load, 54, load.length() > 5 ? 5 : 6, SOFT_WHITE);
  }
  centered("LOAD NOW", 113, 2, MUTED);

  tft->setTextSize(2);
  tft->setTextColor(TFT_CYAN, TFT_BLACK);
  tft->setCursor(30, 150);
  tft->print("TODAY ");
  if (isnan(inverterEnergyToday)) tft->print("--");
  else tft->printf("%.2f", inverterEnergyToday);
  tft->print(" kWh");

  tft->setTextColor(SOFT_WHITE, TFT_BLACK);
  tft->setCursor(30, 181);
  tft->print("FROM BATTERY");

  tft->setCursor(30, 212);
  if (!chargerOn) {
    tft->setTextColor(MUTED, TFT_BLACK);
    tft->print("CHARGER OFF");
  } else {
    tft->setTextColor(GREEN, TFT_BLACK);
    tft->print("CHARGE ");
    if (isnan(chargerWatts)) tft->print("ON");
    else tft->printf("%.0f W", chargerWatts);
  }
}

void drawCharger() {
  if (screenDirty) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
    tft->fillRoundRect(12, 16, 7, 208, 4, chargerOn ? GREEN : MUTED);
    centered(chargerOn ? "CHARGING" : "CHARGER OFF", 14, 2,
             chargerOn ? GREEN : MUTED);
    chargerDirty = true;
  }
  if (!chargerDirty) return;
  chargerDirty = false;

  tft->fillRect(25, 43, 210, 190, TFT_BLACK);
  if (isnan(chargerWatts)) centered("-- W", 54, 6, SOFT_WHITE);
  else {
    const String power = String(chargerWatts, 0) + " W";
    centered(power, 54, power.length() > 5 ? 5 : 6,
             chargerOn ? GREEN : SOFT_WHITE);
  }
  centered("CHARGE POWER", 113, 2, MUTED);

  tft->setTextSize(2);
  tft->setTextColor(TFT_CYAN, TFT_BLACK);
  tft->setCursor(30, 150);
  tft->print("ENERGY ");
  if (isnan(chargerEnergyToday)) tft->print("--");
  else tft->printf("%.2f", chargerEnergyToday);
  tft->print(" kWh");

  tft->setTextColor(SOFT_WHITE, TFT_BLACK);
  tft->setCursor(30, 181);
  tft->print("FROM GRID");

  tft->setCursor(30, 212);
  tft->print("TO BATTERY");
}

void drawArrow(int cx, int cy, bool down, uint16_t color) {
  const int direction = down ? 1 : -1;
  tft->fillTriangle(cx - 15, cy - 6 * direction, cx + 15,
                   cy - 6 * direction, cx, cy + 12 * direction, color);
}

void drawElevator() {
  // Never leave a locally initiated call overlay stuck forever when HA does not
  // produce a new lift state (for example when the lift is already idle).
  if (liftCallActive && millis() - liftCallStarted > 45000) {
    liftCallActive = false;
    returnToSelectedScreen();
    return;
  }
  String title = liftState == "idle" ? "IDLE" : "CALLING";
  uint16_t accent = ORANGE;
  if (liftState == "idle") accent = MUTED;
  if (liftState == "moving") title = "MOVING";
  if (liftState == "stopped") title = "STOPPED";
  if (liftState == "arrived") {
    title = "ARRIVED";
    accent = GREEN;
  }

  if (screenDirty) {
    screenDirty = false;
    tft->fillScreen(TFT_BLACK);
    tft->fillRoundRect(12, 18, 7, 204, 4, accent);
    centered("LIFT", 18, 2, MUTED);
    elevatorDirty = true;
  }
  if (elevatorDirty) {
    tft->fillRect(25, 43, 130, 112, TFT_BLACK);
    tft->fillRect(25, 160, 210, 62, TFT_BLACK);
    const String floorText = String(liftFloor);
    tft->setTextSize(10);
    tft->setTextColor(SOFT_WHITE, TFT_BLACK);
    tft->setCursor(91 - tft->textWidth(floorText) / 2, 50);
    tft->print(floorText);
    centered(title, 174, 3, accent);
    elevatorDirty = false;
  }

  if (liftState == "arrived" || liftDirection == "none" ||
      (liftDirection != "up" && liftDirection != "down") ||
      millis() - lastLiftFrame < 33) return;
  lastLiftFrame = millis();
  const float phase = (millis() - screenStarted) / 330.0f;
  const int bob = static_cast<int>(8.0f * sinf(phase));
  liftAnimation->fillSprite(TFT_BLACK);
  const int direction = liftDirection != "up" ? 1 : -1;
  const int cy = 35 + bob;
  liftAnimation->fillTriangle(5, cy - 11 * direction, 59,
                              cy - 11 * direction, 32,
                              cy + 22 * direction, accent);
  liftAnimation->pushSprite(164, 62);
}

void drawMotion() {
  const uint32_t elapsed = millis() - screenStarted;
  if (elapsed > 6500) {
    returnToSelectedScreen();
    return;
  }
  if (!screenDirty) return;
  screenDirty = false;
  tft->fillScreen(TFT_BLACK);
  const int pulse = 31;
  tft->drawCircle(120, 78, pulse, BLUE);
  tft->drawCircle(120, 78, pulse + 8, NAVY);
  tft->fillCircle(120, 78, 8, BLUE);
  centered("MOTION", 128, 3, SOFT_WHITE);
  centered(motionLocation, 168, 2, MUTED);
}
}  // namespace

namespace display {
void begin() {
  tft = new TFT_eSPI();
  liftAnimation = new TFT_eSprite(tft);
  pinMode(PIN_TFT_BACKLIGHT, OUTPUT);
  analogWriteRange(1023);
  analogWriteFreq(20000);
  applyBrightness();
  tft->init();
  tft->setRotation(0);
  tft->fillScreen(TFT_BLACK);
  liftAnimation->setColorDepth(16);
  liftAnimation->createSprite(96, 82);
}

void showDiagnostic(const char* networkState, const char* address) {
  tft->fillScreen(TFT_BLACK);
  centered("SD_PRO", 31, 3, TFT_CYAN);
  centered("Display OK", 86, 2, TFT_WHITE);
  centered(String(networkState), 122, 2, TFT_WHITE);
  centered(String(address), 156, 2, TFT_YELLOW);
  centered("/status  /update", 205, 1, MUTED);
}

void beginUi() {
  screen = Screen::Boot;
  screenStarted = millis();
  screenDirty = true;
}

void loop() {
  if (millis() - lastFrame < 33) return;
  lastFrame = millis();
  switch (screen) {
    case Screen::Boot: drawBoot(); break;
    case Screen::Clock: drawClock(); break;
    case Screen::Elevator: drawElevator(); break;
    case Screen::EnergyFlow: drawEnergyFlow(); break;
    case Screen::Inverter: drawInverter(); break;
    case Screen::Charger: drawCharger(); break;
    case Screen::Motion: drawMotion(); break;
    case Screen::Alarm: drawAlarm(false); break;
    case Screen::AllClear:
      drawAlarm(true);
      if (millis() - screenStarted > 6000) {
        returnToSelectedScreen();
      }
      break;
  }
}

void setElevatorState(const String& state, int floor, const String& direction,
                      int etaSeconds) {
  const int safeFloor = constrain(floor, -9, 99);
  const int safeEta = max(0, etaSeconds);
  const bool changed = actualLiftState != state || liftFloor != safeFloor ||
                       actualLiftDirection != direction || liftEta != safeEta;
  actualLiftState = state;
  actualLiftDirection = direction;
  liftFloor = safeFloor;
  liftEta = safeEta;
  if (changed) statusDirty = true;

  // Sensor traffic alone never opens the full-screen lift overlay.
  if (!liftCallActive && viewMode != ViewMode::Lift) return;

  const bool entering = screen != Screen::Elevator;
  if (liftCallActive && state == "idle") {
    // The lift may remain idle briefly while the physical call is accepted.
    if (liftState == "moving") liftState = "arrived";
  } else {
    liftState = state;
  }
  liftDirection = direction;
  screen = Screen::Elevator;
  if (entering) screenDirty = true;
  else if (changed) elevatorDirty = true;
  screenStarted = millis();
}

void showLiftCall() {
  liftCallActive = true;
  liftCallStarted = millis();
  liftState = "calling";
  liftDirection = actualLiftDirection;
  screen = Screen::Elevator;
  screenStarted = millis();
  screenDirty = true;
  elevatorDirty = true;
}

void showMotion(const String& location) {
  motionLocation = location;
  screen = Screen::Motion;
  screenStarted = millis();
  screenDirty = true;
}

void setHomeState(const String& condition, float temperature, int humidity,
                  float wind, int airPm25) {
  const bool changed = weatherCondition != condition ||
      (isnan(outsideTemperature) != isnan(temperature)) ||
      (!isnan(temperature) && fabsf(outsideTemperature - temperature) >= 0.05f) ||
      outsideHumidity != humidity || fabsf(windSpeed - wind) >= 0.05f ||
      pm25 != airPm25;
  if (!changed) return;
  weatherCondition = condition;
  outsideTemperature = temperature;
  outsideHumidity = humidity;
  windSpeed = wind;
  pm25 = airPm25;
  weatherDirty = true;
  weatherChangedAt = millis();
}

void setInverterState(bool hasMains, float loadWatts, float energyToday,
                      float outputVoltage, float outputCurrent,
                      bool isChargerOn,
                      float currentChargerWatts) {
  const bool valuesChanged = mainsAvailable != hasMains ||
      (isnan(inverterLoadWatts) != isnan(loadWatts)) ||
      (!isnan(loadWatts) && fabsf(inverterLoadWatts - loadWatts) >= 1.0f) ||
      (isnan(inverterEnergyToday) != isnan(energyToday)) ||
      (!isnan(energyToday) && fabsf(inverterEnergyToday - energyToday) >= 0.005f) ||
      (isnan(inverterOutputVoltage) != isnan(outputVoltage)) ||
      (!isnan(outputVoltage) && fabsf(inverterOutputVoltage - outputVoltage) >= 1.0f) ||
      (isnan(inverterOutputCurrent) != isnan(outputCurrent)) ||
      (!isnan(outputCurrent) && fabsf(inverterOutputCurrent - outputCurrent) >= 0.02f) ||
      chargerOn != isChargerOn ||
      (isnan(chargerWatts) != isnan(currentChargerWatts)) ||
      (!isnan(currentChargerWatts) && fabsf(chargerWatts - currentChargerWatts) >= 1.0f);

  mainsAvailable = hasMains;
  inverterLoadWatts = loadWatts;
  inverterEnergyToday = energyToday;
  inverterOutputVoltage = outputVoltage;
  inverterOutputCurrent = outputCurrent;
  chargerOn = isChargerOn;
  chargerWatts = currentChargerWatts;
  if (valuesChanged) {
    inverterDirty = true;
    flowDirty = true;
  }

  if (viewMode == ViewMode::Auto && !mainsAvailable &&
      screen != Screen::Alarm && screen != Screen::AllClear) {
    if (screen != Screen::Inverter) screenDirty = true;
    screen = Screen::Inverter;
    screenStarted = millis();
  } else if (viewMode == ViewMode::Auto && mainsAvailable &&
             screen == Screen::Inverter) {
    screen = Screen::Clock;
    screenStarted = millis();
    screenDirty = true;
    lastClockMinute = -1;
  }
}

void setChargerState(bool isOn, float powerWatts, float energyKwh,
                     float voltage, float currentAmps) {
  const bool changed = chargerOn != isOn ||
      (isnan(chargerWatts) != isnan(powerWatts)) ||
      (!isnan(powerWatts) && fabsf(chargerWatts - powerWatts) >= 1.0f) ||
      (isnan(chargerEnergyToday) != isnan(energyKwh)) ||
      (!isnan(energyKwh) && fabsf(chargerEnergyToday - energyKwh) >= 0.005f) ||
      (isnan(chargerVoltage) != isnan(voltage)) ||
      (!isnan(voltage) && fabsf(chargerVoltage - voltage) >= 0.5f) ||
      (isnan(chargerCurrent) != isnan(currentAmps)) ||
      (!isnan(currentAmps) && fabsf(chargerCurrent - currentAmps) >= 0.02f);
  chargerOn = isOn;
  chargerWatts = powerWatts;
  chargerEnergyToday = energyKwh;
  chargerVoltage = voltage;
  chargerCurrent = currentAmps;
  if (changed) {
    chargerDirty = true;
    flowDirty = true;
  }
}

void setAlarm(bool active) {
  if (alarmActive == active) return;
  alarmActive = active;
  statusDirty = true;
  screen = active ? Screen::Alarm : Screen::AllClear;
  screenStarted = millis();
  screenDirty = true;
}

void setHaConnected(bool connected) {
  if (haConnected == connected) return;
  haConnected = connected;
  statusDirty = true;
}

bool elevatorVisible() { return screen == Screen::Elevator; }

void nextScreen() {
  if (liftCallActive) {
    liftCallActive = false;
    returnToSelectedScreen();
    return;
  }
  switch (viewMode) {
    case ViewMode::Auto:
    case ViewMode::Home: viewMode = ViewMode::Flow; break;
    case ViewMode::Flow: viewMode = ViewMode::Charger; break;
    case ViewMode::Charger: viewMode = ViewMode::Inverter; break;
    case ViewMode::Inverter: viewMode = ViewMode::Lift; break;
    case ViewMode::Lift: viewMode = ViewMode::Auto; break;
  }
  returnToSelectedScreen();
  lastMetricPage = -1;
}

void cycleBrightness() {
  if (brightnessPercent >= 70) brightnessPercent = 25;
  else if (brightnessPercent >= 25) brightnessPercent = 8;
  else brightnessPercent = 70;
  applyBrightness();
}

void setBrightness(uint8_t percent) {
  brightnessPercent = constrain(percent, 3, 100);
  applyBrightness();
}

uint8_t brightness() { return brightnessPercent; }

bool setViewMode(const String& requested) {
  String mode = requested;
  mode.toLowerCase();
  liftCallActive = false;
  if (mode == "auto") viewMode = ViewMode::Auto;
  else if (mode == "home") viewMode = ViewMode::Home;
  else if (mode == "flow") viewMode = ViewMode::Flow;
  else if (mode == "inverter") viewMode = ViewMode::Inverter;
  else if (mode == "charger") viewMode = ViewMode::Charger;
  else if (mode == "lift") {
    viewMode = ViewMode::Lift;
    liftState = actualLiftState;
    liftDirection = actualLiftDirection;
    elevatorDirty = true;
  }
  else return false;
  returnToSelectedScreen();
  return true;
}

const char* viewModeName() {
  switch (viewMode) {
    case ViewMode::Auto: return "auto";
    case ViewMode::Home: return "home";
    case ViewMode::Flow: return "flow";
    case ViewMode::Inverter: return "inverter";
    case ViewMode::Charger: return "charger";
    case ViewMode::Lift: return "lift";
  }
  return "auto";
}

const char* activeScreenName() {
  switch (screen) {
    case Screen::Boot: return "boot";
    case Screen::Clock: return "home";
    case Screen::Elevator: return "lift";
    case Screen::EnergyFlow: return "flow";
    case Screen::Inverter: return "inverter";
    case Screen::Charger: return "charger";
    case Screen::Motion: return "motion";
    case Screen::Alarm: return "alarm";
    case Screen::AllClear: return "all_clear";
  }
  return "unknown";
}
}  // namespace display
