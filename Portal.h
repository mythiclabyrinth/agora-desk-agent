#pragma once

#include <Arduino.h>
#include <DNSServer.h>

class Portal {
 public:
  void begin();
  void update();
  bool join(const String &ssid, const String &password, unsigned long timeoutMs);
  // Drops the station link shortly after the call, so the HTTP reply that
  // asked for it still reaches the browser; the setup network stays up.
  void forget();
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
  unsigned long _forgetAt = 0;
};

extern Portal portal;
