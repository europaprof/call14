#include "network.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "app_config.h"

namespace {
bool connected = false;

void startRecoveryAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(RECOVERY_AP_SSID, RECOVERY_AP_PASSWORD);
  Serial.printf("Recovery AP: %s, IP: %s\n", RECOVERY_AP_SSID,
                WiFi.softAPIP().toString().c_str());
}
}  // namespace

namespace network {
void begin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(DEVICE_HOSTNAME);

#ifdef RECOVERY_BUILD
  startRecoveryAp();
#endif

  if (WIFI_SSID[0] != '\0') {
#ifdef RECOVERY_BUILD
    WiFi.mode(WIFI_AP_STA);
#else
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to Wi-Fi %s", WIFI_SSID);
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    connected = WiFi.status() == WL_CONNECTED;
  }

  if (!connected) {
#ifndef RECOVERY_BUILD
    startRecoveryAp();
#endif
  }

  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([] { Serial.println("ArduinoOTA update started"); });
  ArduinoOTA.onEnd([] { Serial.println("ArduinoOTA update finished"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("ArduinoOTA error: %u\n", error);
  });
  ArduinoOTA.begin();

  // Europe/Kyiv with automatic EET/EEST daylight-saving transitions.
  configTime("EET-2EEST,M3.5.0/3,M10.5.0/4", "pool.ntp.org",
             "time.cloudflare.com");

  Serial.printf("Mode: %s, address: %s\n", modeName(), address().c_str());
}

void loop() {
  ArduinoOTA.handle();
  connected = WiFi.status() == WL_CONNECTED;
}

bool stationConnected() { return connected; }
const char* modeName() { return connected ? "WiFi" : "Recovery AP"; }
String address() { return connected ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
}  // namespace network
