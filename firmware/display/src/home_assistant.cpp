#include "home_assistant.h"

#if defined(ENABLE_MQTT) && !defined(RECOVERY_BUILD) && !defined(NETWORK_SAFE_BUILD)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "app_config.h"
#include "display.h"

namespace {
WiFiClient* wifiClient = nullptr;
PubSubClient* mqtt = nullptr;
uint32_t lastAttempt = 0;
String condition = "unknown";
float temperature = NAN;
float wind = 0;
int humidity = 0;
int pm25 = -1;
String liftState = "idle";
String liftDirection = "none";
int liftFloor = 14;
bool mainsAvailable = true;
float inverterPower = NAN;
float inverterEnergyToday = NAN;
float inverterVoltage = NAN;
float inverterCurrent = NAN;
bool chargerOn = false;
float chargerPower = NAN;
float chargerEnergy = NAN;
float chargerVoltage = NAN;
float chargerCurrent = NAN;

void updateHome() {
  display::setHomeState(condition, temperature, humidity, wind, pm25);
}

float numberOrNan(String value) {
  value.toLowerCase();
  if (value == "unknown" || value == "unavailable" || value.length() == 0)
    return NAN;
  return value.toFloat();
}

void updateInverter() {
  display::setInverterState(mainsAvailable, inverterPower,
                            inverterEnergyToday, inverterVoltage,
                            inverterCurrent,
                            chargerOn, chargerPower);
}

void updateCharger() {
  display::setChargerState(chargerOn, chargerPower, chargerEnergy,
                           chargerVoltage, chargerCurrent);
}

void receive(char* topic, uint8_t* bytes, unsigned int length) {
  String payload;
  payload.reserve(length);
  for (unsigned int i = 0; i < length; ++i) payload += char(bytes[i]);
  const String name = String(topic).substring(strlen(MQTT_BASE_TOPIC));

  if (name == "weather/condition") condition = payload;
  else if (name == "weather/temperature") temperature = payload.toFloat();
  else if (name == "weather/humidity") humidity = payload.toInt();
  else if (name == "weather/wind") wind = payload.toFloat();
  else if (name == "air/pm25") pm25 = payload.toInt();
  else if (name == "alarm") {
    payload.toLowerCase();
    display::setAlarm(payload == "on" || payload == "1" || payload == "true");
    return;
  } else if (name == "lift/state") liftState = payload;
  else if (name == "lift/direction") liftDirection = payload;
  else if (name == "lift/floor") liftFloor = payload.toInt();
  else if (name == "power/mains") {
    payload.toLowerCase();
    mainsAvailable = payload == "on" || payload == "1" || payload == "true";
  } else if (name == "inverter/power") inverterPower = numberOrNan(payload);
  else if (name == "inverter/energy_today") {
    const float value = numberOrNan(payload);
    // Sonoff goes unavailable when inverter power disappears. Keep the last
    // valid daily total so USED TODAY remains meaningful on the display.
    if (!isnan(value)) inverterEnergyToday = value;
  }
  else if (name == "inverter/voltage") inverterVoltage = numberOrNan(payload);
  else if (name == "inverter/current") inverterCurrent = numberOrNan(payload);
  else if (name == "charger/state") {
    payload.toLowerCase();
    chargerOn = payload == "on" || payload == "1" || payload == "true";
  } else if (name == "charger/power") chargerPower = numberOrNan(payload);
  else if (name == "charger/energy") chargerEnergy = numberOrNan(payload);
  else if (name == "charger/voltage") chargerVoltage = numberOrNan(payload);
  else if (name == "charger/current") chargerCurrent = numberOrNan(payload);

  if (name.startsWith("weather/") || name == "air/pm25") updateHome();
  if (name.startsWith("lift/"))
    display::setElevatorState(liftState, liftFloor, liftDirection, 0);
  if (name.startsWith("power/") || name.startsWith("inverter/") ||
      name.startsWith("charger/")) updateInverter();
  if (name.startsWith("charger/")) updateCharger();
}

void connect() {
  if (!mqtt || mqtt->connected() || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastAttempt < 10000) return;
  lastAttempt = millis();
  String id = String(DEVICE_HOSTNAME) + '-' + String(ESP.getChipId(), HEX);
  const bool connected = MQTT_USERNAME[0] != '\0'
      ? mqtt->connect(id.c_str(), MQTT_USERNAME, MQTT_PASSWORD)
      : mqtt->connect(id.c_str());
  if (connected) {
    const String subscription = String(MQTT_BASE_TOPIC) + '#';
    mqtt->subscribe(subscription.c_str());
    display::setHaConnected(true);
    Serial.println("MQTT connected");
  } else {
    Serial.printf("MQTT unavailable, state=%d\n", mqtt->state());
  }
}
}

namespace homeAssistant {
void begin() {
  wifiClient = new WiFiClient();
  wifiClient->setTimeout(1000);
  mqtt = new PubSubClient(*wifiClient);
  mqtt->setServer(MQTT_HOST, MQTT_PORT);
  mqtt->setCallback(receive);
  mqtt->setSocketTimeout(1);
  mqtt->setBufferSize(256);
}
void loop() {
  connect();
  if (mqtt && mqtt->connected()) mqtt->loop();
  else display::setHaConnected(false);
}
void callLift() {
  if (mqtt && mqtt->connected()) mqtt->publish(MQTT_CALL_TOPIC, "PRESS");
  display::showLiftCall();
}
bool connected() { return mqtt && mqtt->connected(); }
int state() { return mqtt ? mqtt->state() : -99; }
}
#else
namespace homeAssistant {
void begin() {}
void loop() {}
void callLift() {}
bool connected() { return false; }
int state() { return -99; }
}
#endif
