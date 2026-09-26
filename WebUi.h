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
  void handleWifiForget();
  void handleWifiScan();
  void handleAgentsGet();
  void handleAgentsPost();
  void handleAgentChannels();
  void handleChat();
  void handleVoiceGet();
  void handleVoicePost();
  void handleVoiceKeys();
  void handleVoiceTest();
  void handleVoiceTalk();
  void handleVoiceWake();
  String wakeJson();
  void handleTranscribeUpload();
  void handleTranscribe();
  void handleSay();
  void handleTone();
  void dropClip();
  void handleCaptive();
  void handleNotFound();
  void sendJson(int code, const String &body);
  void sendError(int code, const char *message);

  WebServer _server{ 80 };

  // A clip the browser is uploading for transcription, gathered in PSRAM.
  uint8_t *_clip = nullptr;
  size_t _clipLen = 0;
  size_t _clipCap = 0;
  bool _clipTooBig = false;
  String _clipName;
  String _clipType;
};

extern WebUi webUi;
