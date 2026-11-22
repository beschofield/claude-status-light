#include "net.h"

#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "certs.h"
#include "config.h"
#include "looks.h"
#include "settings.h"

volatile char pendingMode = 0;

enum HubState : uint8_t { HUB_UNSET, HUB_OK, HUB_DOWN, HUB_BAD_TOKEN };
static volatile HubState hubState = HUB_UNSET;
static volatile int hubFails = 0;

static WiFiManager wm;
static WebServer web(80);
static bool webRunning = false;
static bool settingsDirty = false;

// Setup-screen fields (shown under the Wi-Fi password on the phone page).
static WiFiManagerParameter pName("name", "Light name (becomes NAME.local)", "", 32);
static WiFiManagerParameter pHub("hub", "Hub address, e.g. https://charon.xxx.ts.net/status-light", "", 160);
static WiFiManagerParameter pToken("token", "Hub token", "", 80, "type='password'");
static WiFiManagerParameter pChannel("channel", "Channel (home, work, ...)", "", 32);

static void saveParams() {
  settings.name = pName.getValue();
  settings.hub = pHub.getValue();
  if (strlen(pToken.getValue())) settings.token = pToken.getValue();  // blank = keep the old one
  settings.channel = pChannel.getValue();
  settingsSave();
  settingsDirty = true;  // restart once Wi-Fi is back, so everything uses the new values
  Serial.printf("settings saved: name=%s hub=%s channel=%s token=%s\n", settings.name.c_str(),
                settings.hub.c_str(), settings.channel.c_str(), settings.token.length() ? "set" : "none");
}

static String urlEncode(const String& s) {
  String out;
  for (char c : s) {
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.') {
      out += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof buf, "%%%02X", (uint8_t)c);
      out += buf;
    }
  }
  return out;
}

// Long-polls the hub forever in its own task, so animations never stutter.
static void hubTask(void*) {
  NetworkClientSecure tls;
  tls.setCACert(HUB_ROOT_CAS);
  NetworkClient plain;
  HTTPClient http;
  http.setReuse(true);  // keep one connection open: a TLS handshake per poll is slow
  http.setConnectTimeout(8000);
  http.setTimeout((HUB_WAIT_S + 15) * 1000);
  char have = '?';
  uint32_t backoff = 2000;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED || settings.hub.isEmpty() || wm.getConfigPortalActive()) {
      hubState = settings.hub.isEmpty() ? HUB_UNSET : hubState;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    String url = settings.hub + "/api/light?channel=" + urlEncode(settings.channel) +
                 "&have=" + String(have) + "&wait=" + String(HUB_WAIT_S);
    bool began = url.startsWith("https://") ? http.begin(tls, url) : http.begin(plain, url);
    int code = -1;
    String body;
    if (began) {
      http.addHeader("Authorization", "Bearer " + settings.token);
      code = http.GET();
      if (code == 200) body = http.getString();
      http.end();
    }
    if (code == 200 && body.length() && isStatusMode(body[0])) {
      have = body[0];
      pendingMode = have;
      hubFails = 0;
      hubState = HUB_OK;
      backoff = 2000;
      continue;
    }
    have = '?';  // after an error, ask for the current mode straight away next time
    hubFails = hubFails + 1;
    if (code == 401) hubState = HUB_BAD_TOKEN;
    else if (hubFails >= HUB_FAILS_BEFORE_WARNING) hubState = HUB_DOWN;
    Serial.printf("hub: poll failed (%d %s), retry in %lus\n", code,
                  code < 0 ? HTTPClient::errorToString(code).c_str() : "", backoff / 1000);
    vTaskDelay(pdMS_TO_TICKS(backoff));
    backoff = min<uint32_t>(backoff * 2, 30000);
  }
}

static const char* modeName(char m) {
  switch (m) {
    case 'w': return "working";
    case 'n': return "needs you";
    case 'r': return "ready";
    default: return "idle";
  }
}

static const char* hubName() {
  switch (hubState) {
    case HUB_OK: return "connected";
    case HUB_DOWN: return "unreachable";
    case HUB_BAD_TOKEN: return "token rejected";
    default: return "not set";
  }
}

static void startWeb() {
  web.on("/", HTTP_GET, [] {
    String html = "<!doctype html><meta name=viewport content='width=device-width'>"
                  "<title>" + settings.name + "</title><body style='font:16px sans-serif;margin:2em'>"
                  "<h2>" + settings.name + "</h2><p>Showing: <b>" + modeName(looksMode()) +
                  "</b><br>Channel: " + settings.channel + "<br>Hub: " + hubName() +
                  "<br>Ring: " + (RGBW ? "RGBW" : "RGB") + "</p><p>Try: ";
    for (const char* m : {"w", "n", "r", "i"}) {
      html += String("<a href='/mode?m=") + m + "'>" + modeName(m[0]) + "</a> ";
    }
    html += "</p><p><small>The hub takes over again at its next change.</small></p>";
    web.send(200, "text/html", html);
  });
  web.on("/mode", [] {
    String m = web.arg("m");
    if (m.length() == 1 && isStatusMode(m[0])) {
      pendingMode = m[0];
      web.sendHeader("Location", "/");
      web.send(303);
    } else {
      web.send(400, "text/plain", "m must be one of w n r i\n");
    }
  });
  web.on("/status", HTTP_GET, [] {
    web.send(200, "application/json",
             String("{\"name\":\"") + settings.name + "\",\"mode\":\"" + looksMode() +
                 "\",\"channel\":\"" + settings.channel + "\",\"hub\":\"" + hubName() +
                 "\",\"rgbw\":" + (RGBW ? "true" : "false") + "}\n");
  });
  web.begin();
  MDNS.begin(settings.name.c_str());
  MDNS.addService("http", "tcp", 80);
  webRunning = true;
  Serial.printf("wifi: connected, %s at http://%s.local (%s)\n", WiFi.SSID().c_str(),
                settings.name.c_str(), WiFi.localIP().toString().c_str());
}

void netOpenSetup() {
  if (wm.getConfigPortalActive()) return;
  if (webRunning) {
    web.stop();
    MDNS.end();
    webRunning = false;
  }
  Serial.printf("setup: join the \"%s\" Wi-Fi from a phone\n", SETUP_NETWORK);
  wm.startConfigPortal(SETUP_NETWORK);
}

void netBegin() {
  pName.setValue(settings.name.c_str(), 32);
  pHub.setValue(settings.hub.c_str(), 160);
  pChannel.setValue(settings.channel.c_str(), 32);
  wm.addParameter(&pName);
  wm.addParameter(&pHub);
  wm.addParameter(&pToken);
  wm.addParameter(&pChannel);
  wm.setSaveParamsCallback(saveParams);
  wm.setSaveConfigCallback(saveParams);
  wm.setConfigPortalBlocking(false);  // keep animating while the setup screen is open
  wm.setTitle("Status light");
  wm.setClass("invert");
  std::vector<const char*> menu = {"wifi", "exit"};
  wm.setMenu(menu);
  wm.setConnectTimeout(20);
  wm.setHostname(settings.name.c_str());
  if (!wm.autoConnect(SETUP_NETWORK)) {
    Serial.printf("wifi: no saved network reachable; setup screen open on \"%s\"\n", SETUP_NETWORK);
  }
  xTaskCreate(hubTask, "hub", 10240, nullptr, 1, nullptr);
}

void netLoop() {
  wm.process();
  bool portal = wm.getConfigPortalActive();
  bool online = WiFi.status() == WL_CONNECTED;

  if (settingsDirty && online && !portal) {
    Serial.println("settings changed: restarting");
    delay(300);
    ESP.restart();
  }
  if (online && !portal && !webRunning) startWeb();
  if (!online && webRunning) {
    web.stop();
    MDNS.end();
    webRunning = false;
  }
  if (webRunning) web.handleClient();

  SystemLook look = SYS_NONE;
  if (portal) look = SYS_SETUP;
  else if (!online) look = SYS_CONNECTING;
  else if (hubState == HUB_BAD_TOKEN) look = SYS_BAD_TOKEN;
  else if (hubState == HUB_DOWN) look = SYS_OFFLINE;
  looksSetSystem(look);
}

String netStatus() {
  String s = String("name ") + settings.name + ", ring " + (RGBW ? "RGBW" : "RGB") + ", showing " +
             modeName(looksMode()) + "; wifi ";
  if (wm.getConfigPortalActive()) s += "setup screen open";
  else if (WiFi.status() == WL_CONNECTED) s += WiFi.SSID() + " " + WiFi.localIP().toString();
  else s += "not connected";
  s += "; hub " + String(hubName());
  if (settings.hub.length()) s += " (" + settings.hub + ", channel " + settings.channel + ")";
  return s;
}
