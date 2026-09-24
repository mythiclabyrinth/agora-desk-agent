#pragma once

#include <WebServer.h>

class WebUi {
public:
  void begin();
  void handle();

private:
  void handleIndex();
  void handleStatus();
  void handleListen();
  void handleWifi();
  void handleWifiScan();
  void handleAgentsGet();
  void handleAgentsPost();
  void handleChat();
  void handleCaptive();
  void handleNotFound();
  void sendJson(int code, const String &body);
  void sendError(int code, const char *message);

  WebServer _server{ 80 };
};

extern WebUi webUi;
