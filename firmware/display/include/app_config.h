#pragma once

#if __has_include("secrets.h")
#include "secrets.h"
#else
// The firmware still builds and exposes its recovery access point.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define OTA_PASSWORD "change-me"
#define RECOVERY_AP_PASSWORD "change-me-now"
#define MQTT_HOST ""
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#endif

constexpr char DEVICE_HOSTNAME[] = "sd-pro-lift-display";
constexpr char RECOVERY_AP_SSID[] = "SD-PRO-Recovery";

constexpr char MQTT_BASE_TOPIC[] = "sdpro/display/";
constexpr char MQTT_CALL_TOPIC[] = "sdpro/display/call_lift";
