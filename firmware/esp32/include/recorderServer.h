#ifndef RECORDERSERVER_H
#define RECORDERSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include "sdCard.h"
#include "recorderPreferences.h"
#include <ArduinoJson.h>


class RecorderServer {
public:
    RecorderServer(RecorderPreferences* preferences, SDCard* sdCard);
    ~RecorderServer();
    
    void start();
    void stop();
    void run();
    bool isRunning() const { return running; }

private:
    WebServer server;
    RecorderPreferences* preferences;
    SDCard* sdCard;
    bool running;
    
    // Request handlers
    void handleRoot();
    void handleGetSettingsApi();
    void handlePostSettingsApi();
    void handleNotFound();
    void handleSettings();
    
    // Helper methods
    String getSettingsJson();
    void setSettingsFromJson(const String& json);
    String getHtmlPage();
    String getSettingsHtmlPage();
};

#endif
