#ifndef RECORDERSERVER_H
#define RECORDERSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include "sdCard.h"
#include "recorderPreferences.h"
#include "rtc.h"
#include <ArduinoJson.h>
#include <functional>


class RecorderServer {
public:
    RecorderServer(RecorderPreferences* preferences, SDCard* sdCard, RTC* rtc);
    ~RecorderServer();
    
    void start();
    void stop();
    void run();
    bool isRunning() const { return running; }
    void setRecordingCallbacks(std::function<void()> startCb,
                               std::function<void()> stopCb,
                               std::function<bool()> isRecCb);

private:
    WebServer server;
    RecorderPreferences* preferences;
    SDCard* sdCard;
    RTC* rtc;
    bool running;

    std::function<void()> onStartRecording;
    std::function<void()> onStopRecording;
    std::function<bool()> getIsRecording;
    
    // Request handlers
    void handleRoot();
    void handleGetSettingsApi();
    void handlePostSettingsApi();
    void handleNotFound();
    void handleSettings();
    void getFilesHtmlPage();
    void handleDownload();
    void handleDeleteFilesApi();
    void handleStartRecordingApi();
    void handleStopRecordingApi();
    void handleGetRecordingStatusApi();
    void handleRestartApi();
    void handleSetTimeApi();
    void handleGetTimeApi();
    
    // Helper methods
    String getSettingsJson();
    void setSettingsFromJson(const String& json);
    String getHomeHtmlPage();
    String getSettingsHtmlPage();
};

#endif
