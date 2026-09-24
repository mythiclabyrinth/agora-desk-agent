#pragma once

#include <Arduino.h>
#include <DNSServer.h>

class Portal {
 public:
  void begin();
  void update();
  bool join(const String &ssid, const String &password, unsigned long timeoutMs);
  // Nearby networks as JSON. Works in AP+STA, including before the board has joined.
  String scanJson();

  bool staConnected();
  String staIp() const;
  String apIp() const;

 private:
  void startAccessPoint();
  void startMdns();

  DNSServer _dns;
  bool _ap = false;
  bool _dnsOn = false;
  bool _mdns = false;
  unsigned long _lastReconnectAt = 0;
};

extern Portal portal;
