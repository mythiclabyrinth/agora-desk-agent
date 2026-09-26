#include "Portal.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "Board.h"
#include "Config.h"
#include "Json.h"

Portal portal;

void Portal::begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(MDNS_HOST);
  startAccessPoint();

  String ssid = configStore.wifiSsid();
  if (!ssid.length()) {
    Serial.println("No saved Wi-Fi. Join the setup network.");
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), configStore.wifiPassword().c_str());
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_BOOT_WAIT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi ");
    Serial.println(WiFi.localIP());
    startMdns();
  } else {
    Serial.println("Wi-Fi not up yet. The setup network is still available.");
  }
}

void Portal::startAccessPoint() {
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Setup network failed to start.");
    return;
  }
  _ap = true;
  Serial.print("Setup network ");
  Serial.print(AP_SSID);
  Serial.print(" at ");
  Serial.println(WiFi.softAPIP());
}

void Portal::startMdns() {
  if (_mdns) return;
  _mdns = MDNS.begin(MDNS_HOST);
  if (_mdns) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("Page at http://");
    Serial.print(MDNS_HOST);
    Serial.println(".local");
  }
}

String Portal::scanJson() {
  // AP+STA can scan without dropping the setup network. The call blocks for
  // a few seconds, which is why the page asks for it instead of scanning always.
  int found = WiFi.scanNetworks(false, false);
  if (found < 0) {
    WiFi.scanDelete();
    return "{\"error\":\"Could not scan for networks. Try again.\"}";
  }

  struct NetHit {
    String ssid;
    int32_t rssi;
    bool open;
  };
  NetHit hits[12];
  int count = 0;
  for (int i = 0; i < found; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length() || ssid == AP_SSID) continue;
    int32_t rssi = WiFi.RSSI(i);
    bool open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    int slot = -1;
    for (int j = 0; j < count; j++) {
      if (hits[j].ssid == ssid) {
        slot = j;
        break;
      }
    }
    if (slot >= 0) {
      if (rssi > hits[slot].rssi) {
        hits[slot].rssi = rssi;
        hits[slot].open = open;
      }
      continue;
    }
    if (count >= 12) {
      int weakest = 0;
      for (int j = 1; j < count; j++) {
        if (hits[j].rssi < hits[weakest].rssi) weakest = j;
      }
      if (rssi <= hits[weakest].rssi) continue;
      hits[weakest] = NetHit{ssid, rssi, open};
      continue;
    }
    hits[count++] = NetHit{ssid, rssi, open};
  }
  WiFi.scanDelete();

  for (int i = 1; i < count; i++) {
    NetHit key = hits[i];
    int j = i;
    while (j > 0 && hits[j - 1].rssi < key.rssi) {
      hits[j] = hits[j - 1];
      j--;
    }
    hits[j] = key;
  }

  String body = "{\"networks\":[";
  for (int i = 0; i < count; i++) {
    if (i) body += ',';
    body += "{\"ssid\":\"";
    body += jsonEscape(hits[i].ssid);
    body += "\",\"rssi\":";
    body += String(hits[i].rssi);
    body += ",\"open\":";
    body += hits[i].open ? "true" : "false";
    body += '}';
  }
  body += "]}";
  return body;
}

bool Portal::join(const String &ssid, const String &password, unsigned long timeoutMs) {
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < timeoutMs) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) startMdns();
  return WiFi.status() == WL_CONNECTED;
}

void Portal::forget() {
  _forgetAt = millis() + WIFI_FORGET_DELAY_MS;
}

void Portal::update() {
  if (_forgetAt && static_cast<long>(millis() - _forgetAt) >= 0) {
    _forgetAt = 0;
    WiFi.disconnect(false, false);
    Serial.println("Wi-Fi forgotten; setup network only");
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (_dnsOn) {
      _dns.stop();
      _dnsOn = false;
    }
    startMdns();
  } else if (_ap) {
    if (!_dnsOn) {
      _dns.start(53, "*", WiFi.softAPIP());
      _dnsOn = true;
    }
    if (_dnsOn) _dns.processNextRequest();

    String ssid = configStore.wifiSsid();
    if (ssid.length() && millis() - _lastReconnectAt > 15000) {
      _lastReconnectAt = millis();
      WiFi.reconnect();
    }
  }
}

bool Portal::staConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String Portal::staIp() const {
  if (WiFi.status() != WL_CONNECTED) return "";
  return WiFi.localIP().toString();
}

String Portal::apIp() const {
  if (!_ap) return "";
  return WiFi.softAPIP().toString();
}
