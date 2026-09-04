#include "web_portal.h"

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Updater.h>

#include "app_config.h"
#include "display.h"
#include "home_assistant.h"
#include "network.h"

namespace {
ESP8266WebServer server(80);

const char UPDATE_FORM[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width">
<title>SD_PRO Update</title></head><body><h1>Firmware update</h1>
<form method=POST action=/update enctype=multipart/form-data>
<input type=file name=firmware accept=.bin required><button>Upload</button></form>
<p><a href=/status>Status</a></p></body></html>)HTML";

const char SETTINGS_PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width">
<title>SD PRO</title><style>
*{box-sizing:border-box}body{font:17px system-ui;background:#090a0c;color:#f5f5f7;
max-width:620px;margin:0 auto;padding:22px}h1{font-size:29px;margin:6px 0}h2{font-size:17px;
color:#9a9aa1;margin:26px 0 12px}.sub{color:#8e8e93;margin:0 0 22px}.card{background:#17181c;
border:1px solid #292a2f;border-radius:20px;padding:18px;margin:14px 0}.grid{display:grid;
grid-template-columns:repeat(2,1fr);gap:10px}.mode,.quick{border:0;border-radius:14px;
background:#292a30;color:#fff;padding:15px 8px;font:600 16px system-ui}.mode.active{background:#ff8a00;
color:#090a0c}.mode.auto.active{background:#30d158}.readout{display:flex;justify-content:space-between;
align-items:center;margin-bottom:14px}.pill{background:#292a30;border-radius:99px;padding:6px 11px;
text-transform:uppercase;color:#ff9f0a;font-size:13px}input{width:100%;accent-color:#ff9f0a}.quickrow{
display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-top:14px}.quick{padding:10px 4px;
font-size:14px}a{color:#64a8ff;text-decoration:none}.links{display:flex;gap:18px;flex-wrap:wrap;
margin:24px 3px}</style></head><body><h1>SD PRO Display</h1>
<p class=sub>Active screen: <b id=active>%ACTIVE%</b></p>
<div class=card><div class=readout><b>Screen mode</b><span class=pill id=current>%MODE%</span></div>
<div class=grid>
<button class="mode auto" data-mode=auto>AUTO</button><button class=mode data-mode=home>HOME</button>
<button class=mode data-mode=flow>FLOW</button>
<button class=mode data-mode=charger>CHARGER</button><button class=mode data-mode=inverter>INVERTER</button>
<button class=mode data-mode=lift>LIFT</button></div></div>
<div class=card><div class=readout><b>Brightness</b><b><span id=v></span>%</b></div>
<input id=b type=range min=3 max=100><div class=quickrow>
<button class=quick data-value=8>Night</button><button class=quick data-value=25>25%</button>
<button class=quick data-value=70>70%</button><button class=quick data-value=100>Max</button></div></div>
<div class=links><a href=/status>JSON status</a><a href=/update>Firmware update</a></div>
<script>const mode='%MODE%',b=document.querySelector('#b'),v=document.querySelector('#v');
document.querySelectorAll('.mode').forEach(x=>{if(x.dataset.mode===mode)x.classList.add('active');
x.onclick=async()=>{await fetch('/api/screen?mode='+x.dataset.mode);location.reload()}});
b.value=%BRIGHTNESS%;v.textContent=b.value;b.oninput=()=>v.textContent=b.value;
b.onchange=()=>fetch('/api/brightness?value='+b.value);
document.querySelectorAll('.quick').forEach(x=>x.onclick=()=>{b.value=x.dataset.value;
v.textContent=b.value;fetch('/api/brightness?value='+b.value)});</script></body></html>)HTML";

void sendStatus() {
  String json = F("{\"device\":\"");
  json += DEVICE_HOSTNAME;
  json += F("\",\"mode\":\"");
  json += network::modeName();
  json += F("\",\"ip\":\"");
  json += network::address();
  json += F("\",\"rssi\":");
  json += network::stationConnected() ? String(WiFi.RSSI()) : F("null");
  json += F(",\"uptime_ms\":");
  json += millis();
  json += F(",\"free_heap\":");
  json += ESP.getFreeHeap();
  json += F(",\"mqtt\":");
  json += homeAssistant::connected() ? F("true") : F("false");
  json += F(",\"mqtt_state\":");
  json += homeAssistant::state();
  json += F(",\"brightness\":");
  json += display::brightness();
  json += F(",\"view_mode\":\"");
  json += display::viewModeName();
  json += F("\",\"active_screen\":\"");
  json += display::activeScreenName();
  json += '"';
  json += '}';
  server.send(200, "application/json", json);
}

bool requireUpdateAuth() {
  if (server.authenticate("admin", OTA_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}
}  // namespace

namespace webPortal {
void begin() {
  server.on("/", HTTP_GET, [] { server.sendHeader("Location", "/settings", true); server.send(302); });
  server.on("/status", HTTP_GET, sendStatus);
  server.on("/settings", HTTP_GET, [] {
    String page = FPSTR(SETTINGS_PAGE);
    page.replace("%BRIGHTNESS%", String(display::brightness()));
    page.replace("%MODE%", display::viewModeName());
    page.replace("%ACTIVE%", display::activeScreenName());
    server.send(200, "text/html", page);
  });
  server.on("/update", HTTP_GET, [] {
    if (!requireUpdateAuth()) return;
    server.send_P(200, "text/html", UPDATE_FORM);
  });
#if !defined(RECOVERY_BUILD) && !defined(NETWORK_SAFE_BUILD)
  server.on("/api/elevator", HTTP_GET, [] {
    const String state = server.hasArg("state") ? server.arg("state") : "calling";
    const int floor = server.hasArg("floor") ? server.arg("floor").toInt() : 14;
    const String direction = server.hasArg("direction") ? server.arg("direction") : "down";
    const int eta = server.hasArg("eta") ? server.arg("eta").toInt() : 0;
    display::setElevatorState(state, floor, direction, eta);
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/motion", HTTP_GET, [] {
    display::showMotion(server.hasArg("location") ? server.arg("location") : "Floor 14");
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/brightness", HTTP_GET, [] {
    const int requested = constrain(server.arg("value").toInt(), 3, 100);
    display::setBrightness(requested);
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/screen", HTTP_GET, [] {
    if (!server.hasArg("mode") || !display::setViewMode(server.arg("mode"))) {
      server.send(400, "application/json", "{\"error\":\"mode must be auto, home, flow, charger, inverter or lift\"}");
      return;
    }
    String reply = F("{\"ok\":true,\"mode\":\"");
    reply += display::viewModeName();
    reply += F("\",\"screen\":\"");
    reply += display::activeScreenName();
    reply += F("\"}");
    server.send(200, "application/json", reply);
  });
#else
  server.on("/api/elevator", HTTP_ANY, [] {
    server.send(503, "application/json", "{\"error\":\"display disabled in recovery\"}");
  });
  server.on("/api/motion", HTTP_ANY, [] {
    server.send(503, "application/json", "{\"error\":\"display disabled in recovery\"}");
  });
#endif
  server.on("/update", HTTP_POST,
            [] {
              if (!requireUpdateAuth()) return;
              const bool ok = !Update.hasError();
              server.send(200, "text/plain", ok ? "Update complete; rebooting" : "Update failed");
              if (ok) {
                delay(250);
                ESP.restart();
              }
            },
            [] {
              if (!server.authenticate("admin", OTA_PASSWORD)) return;
              HTTPUpload& upload = server.upload();
              if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("Web update: %s\n", upload.filename.c_str());
                if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) Update.printError(Serial);
              } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
              } else if (upload.status == UPLOAD_FILE_END) {
                if (!Update.end(true)) Update.printError(Serial);
              }
              yield();
            });
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server started: /status and /update");
}

void loop() { server.handleClient(); }
}  // namespace webPortal
