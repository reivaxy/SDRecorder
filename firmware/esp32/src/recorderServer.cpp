#include "recorderServer.h"
#include "styles.h"
#include "modalStyle.h"
#include "filesStyle.h"
#include "settingsHtmlPage.h"
#include "homeHtmlPage.h"
#include "filesHtmlPage.h"
#include <SD.h>

RecorderServer::RecorderServer(RecorderPreferences* preferences, SDCard* sdCard, RTC* rtc)
    : server(80), preferences(preferences), sdCard(sdCard), rtc(rtc), running(false),
      onStartRecording(nullptr), onStopRecording(nullptr), getIsRecording(nullptr) {
}

RecorderServer::~RecorderServer() {
    stop();
}

void RecorderServer::start() {
    if (running) {
        log_w("Server is already running");
        return;
    }
    
    // Set WiFi mode to Access Point
    WiFi.mode(WIFI_AP);
    
    // Configure AP with default credentials
    String ssid = preferences->getSettingString(PREF_AP_SSID);
    String password = preferences->getSettingString(PREF_AP_PASSWORD);
    log_i("Starting AP with SSID: %s", ssid.c_str());
    
    if (!WiFi.softAP(ssid, password)) {
        log_e("Failed to start AP");
        return;
    }
    
    log_i("AP started: %s", ssid.c_str());
    log_i("IP address: %s", WiFi.softAPIP().toString().c_str());
    
    // Register request handlers
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/settings", HTTP_GET, [this]() { handleSettings(); });
    server.on("/files", HTTP_GET, [this]() { getFilesHtmlPage(); });
    server.on("/download", HTTP_GET, [this]() { handleDownload(); });
    server.on("/apis/settings", HTTP_GET, [this]() { handleGetSettingsApi(); });
    server.on("/apis/settings", HTTP_POST, [this]() { handlePostSettingsApi(); });
    server.on("/apis/recording/start",  HTTP_POST, [this]() { handleStartRecordingApi(); });
    server.on("/apis/recording/stop",   HTTP_POST, [this]() { handleStopRecordingApi(); });
    server.on("/apis/recording/status", HTTP_GET,  [this]() { handleGetRecordingStatusApi(); });
    server.on("/apis/files/delete",     HTTP_POST, [this]() { handleDeleteFilesApi(); });
    server.on("/apis/system/restart",   HTTP_POST, [this]() { handleRestartApi(); });
    server.on("/apis/system/time",      HTTP_POST, [this]() { handleSetTimeApi(); });
    server.on("/apis/system/time",      HTTP_GET,  [this]() { handleGetTimeApi(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    // Start server
    server.begin();
    running = true;
    
    log_i("RecorderServer started on port 80");
}

void RecorderServer::stop() {
    if (!running) {
        return;
    }
    
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    running = false;
    
    log_i("RecorderServer stopped");
}

void RecorderServer::run() {
    if (running) {
        server.handleClient();
    }
}

void RecorderServer::handleRoot() {
    log_i("Handling root request");
    server.send(200, "text/html", getHomeHtmlPage());
}

void RecorderServer::handleSettings() {
    log_i("Handling root request");
    server.send(200, "text/html", getSettingsHtmlPage());
}

void RecorderServer::handleGetSettingsApi() {
    server.send(200, "application/json", getSettingsJson());
}

void RecorderServer::handlePostSettingsApi() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No content\"}");
        return;
    }
    
    String json = server.arg("plain");
    log_i("Settings payload received: %s", json.c_str());
    setSettingsFromJson(json);
    
    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void RecorderServer::handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"Not Found\"}");
}

void RecorderServer::setRecordingCallbacks(std::function<void()> startCb,
                                            std::function<void()> stopCb,
                                            std::function<bool()> isRecCb) {
    onStartRecording = startCb;
    onStopRecording  = stopCb;
    getIsRecording   = isRecCb;
}

void RecorderServer::handleStartRecordingApi() {
    // Check if RTC needs initialization from browser time
    if (rtc && !rtc->isInitialized() && server.hasArg("plain")) {
        String json = server.arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (!error && !doc["timestamp"].isNull()) {
            time_t timestamp = doc["timestamp"].as<time_t>();
            rtc->setTime(timestamp);
            log_i("RTC initialized from browser time during start recording");
        }
    }
    
    if (!onStartRecording) {
        server.send(501, "application/json", "{\"error\":\"Not implemented\"}");
        return;
    }
    onStartRecording();
    server.send(200, "application/json", "{\"status\":\"recording\"}");
}

void RecorderServer::handleStopRecordingApi() {
    if (!onStopRecording) {
        server.send(501, "application/json", "{\"error\":\"Not implemented\"}");
        return;
    }
    onStopRecording();
    server.send(200, "application/json", "{\"status\":\"idle\"}");
}

void RecorderServer::handleGetRecordingStatusApi() {
    bool recording = getIsRecording ? getIsRecording() : false;
    String fileName = recording ? sdCard->getCurrentFileName() : "";
    
    JsonDocument doc;
    doc["recording"] = recording;
    if (recording && fileName.length() > 0) {
        doc["filename"] = fileName;
    }
    
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void RecorderServer::handleRestartApi() {
    log_i("Restart API called");
    server.send(200, "application/json", "{\"status\":\"restarting\"}");
    
    // Save state to restart with server running
    if (running) {
        preferences->setSetting(PREF_RESTART_SERVER, true);
        log_i("Saved restart server preference");
    }
    
    // Delay a bit to allow response to be sent
    delay(500);
    
    // Restart the device
    ESP.restart();
}

void RecorderServer::handleSetTimeApi() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No content\"}");
        return;
    }
    
    String json = server.arg("plain");
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        log_e("JSON parse error: %s", error.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (doc["timestamp"].isNull()) {
        server.send(400, "application/json", "{\"error\":\"Missing timestamp\"}");
        return;
    }
    
    time_t timestamp = doc["timestamp"].as<time_t>();
    
    if (rtc == nullptr) {
        server.send(500, "application/json", "{\"error\":\"RTC not initialized\"}");
        return;
    }
    
    // Only set time if RTC hasn't been initialized yet
    if (!rtc->isInitialized()) {
        rtc->setTime(timestamp);
        log_i("RTC initialized from browser time via time API");
    }
    
    JsonDocument response;
    response["status"] = "success";
    response["time"] = rtc->formatTime();
    
    String responseJson;
    serializeJson(response, responseJson);
    server.send(200, "application/json", responseJson);
}


void RecorderServer::handleGetTimeApi() {
    if (rtc == nullptr) {
        server.send(500, "application/json", "{\"error\":\"RTC not initialized\"}");
        return;
    }
    
    JsonDocument response;
    response["initialized"] = rtc->isInitialized();
    response["timestamp"] = (long)rtc->getTime();
    response["time"] = rtc->formatTime();
    
    String responseJson;
    serializeJson(response, responseJson);
    server.send(200, "application/json", responseJson);
}

void RecorderServer::getFilesHtmlPage() {
    log_i("Handling files request");
    int count = sdCard->getFilesCount();

    String fileListHtml = "";
    
    if (count == 0) {
        fileListHtml += "<p class=\"instruction-text\">No WAV files found on the SD card.</p>\n";
    } else {
        fileListHtml += R"(
            <div class="select-all-container" id="selectAllContainer">
                <input type="checkbox" id="selectAllCheckbox" class="file-checkbox" onchange='toggleSelectAll()'>
                <label for="selectAllCheckbox" style="cursor: pointer; margin-left: 5px; flex-grow: 1;">Select All</label>
                <span class="selected-count" id="selectedCount">0 selected</span>
            </div>
            <div class="file-list-container" id="fileList">)";
        
        for (int i = 0; i < count; i++) {
            const char* fullPath = sdCard->getFileName(i);
            if (!fullPath) continue;
            String fileName = String(fullPath);
            if (fileName.startsWith("/")) fileName = fileName.substring(1);

            // Get file size and modification time
            File f = SD.open(fullPath, FILE_READ);
            String sizeStr = "?";
            String dateStr = "?";
            if (f) {
                size_t sz = f.size();
                time_t modTime = f.getLastWrite();
                f.close();
                
                // Format size
                if (sz >= 1024 * 1024) {
                    sizeStr = String(sz / (1024.0f * 1024.0f), 1) + " MB";
                } else if (sz >= 1024) {
                    sizeStr = String(sz / 1024.0f, 1) + " KB";
                } else {
                    sizeStr = String(sz) + " B";
                }
                
                // Format date
                if (modTime > 0 && rtc != nullptr && rtc->isInitialized()) {
                    // If RTC is available, use it for proper formatting
                    struct tm* timeinfo = localtime(&modTime);
                    char buffer[20];
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
                    dateStr = String(buffer);
                } else if (modTime > 0) {
                    // Fallback: just show the timestamp
                    dateStr = String((long)modTime);
                }
            }

            fileListHtml += "<div class=\"file-item\">"
                    "<input type=\"checkbox\" class=\"file-checkbox file-select\" value=\"" + fileName + "\" onchange='updateSelectedCount()'>"
                    "<div class=\"file-info\">"
                    "<a href=\"/download?file=" + fileName + "\" class=\"file-link\">" + fileName + "</a>"
                    "<span class=\"file-size\">" + sizeStr + "</span>"
                    "<span class=\"file-date\" style=\"display: block; font-size: 0.85em; color: #999; margin-top: 4px;\">" + dateStr + "</span>"
                    "</div>"
                    "</div>\n";
        }
        
        fileListHtml += R"(
            </div>
            <div class="delete-section" id="deleteSection">
                <p id="deleteMessage">Ready to delete selected files.</p>
                <button class="btn-delete-files" onclick='deleteSelectedFiles()'>Delete Selected Files</button>
            </div>)";
    }

    String html = String(R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>SD Recorder - Files</title>)") 
            + String(RECORDER_CSS) + String(MODAL_CSS) + String(FILES_CSS) + String(FILES_HTML_PAGE_TEMPLATE);
    html.replace("%FILE_LIST%", fileListHtml);
    server.send(200, "text/html", html);
}

void RecorderServer::handleDownload() {
    if (!server.hasArg("file")) {
        server.send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
        return;
    }

    String fileName = server.arg("file");

    // Basic path traversal protection
    if (fileName.indexOf("..") >= 0 || fileName.indexOf("/") >= 0) {
        server.send(400, "application/json", "{\"error\":\"Invalid file name\"}");
        return;
    }

    String filePath = "/" + fileName;

    if (!SD.exists(filePath.c_str())) {
        server.send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    File file = SD.open(filePath.c_str(), FILE_READ);
    if (!file) {
        server.send(500, "application/json", "{\"error\":\"Could not open file\"}");
        return;
    }

    server.sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    server.streamFile(file, "audio/wav");
    file.close();
}

void RecorderServer::handleDeleteFilesApi() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No content\"}");
        return;
    }

    String json = server.arg("plain");
    log_i("Delete request payload received: %s", json.c_str());
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        log_e("JSON parsing error: %s", error.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (doc["files"].isNull() || !doc["files"].is<JsonArray>()) {
        server.send(400, "application/json", "{\"error\":\"Missing files array\"}");
        return;
    }
    
    JsonArray filesArray = doc["files"].as<JsonArray>();
    int deletedCount = 0;
    int failedCount = 0;
    
    for (JsonVariant file : filesArray) {
        String fileName = file.as<String>();
        
        // Basic path traversal protection
        if (fileName.indexOf("..") >= 0) {
            log_w("Path traversal attempt detected: %s", fileName.c_str());
            failedCount++;
            continue;
        }
        
        // Ensure path starts with /
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }
        
        if (sdCard->deleteFile(fileName.c_str())) {
            deletedCount++;
        } else {
            failedCount++;
        }
    }
    
    String response = String("{\"deleted\":") + deletedCount + ",\"failed\":" + failedCount + "}";
    server.send(200, "application/json", response);
}

String RecorderServer::getSettingsJson() {
    JsonDocument doc;
    size_t prefCount;
    const RecorderPreferences::SettingMetadata* metadata = RecorderPreferences::getSettingsMetadata(prefCount);
    
    // Create a combined structure with metadata and values
    JsonDocument fullDoc;
    JsonObject metadataObj = fullDoc["metadata"].to<JsonObject>();
    JsonObject valuesObj = fullDoc["values"].to<JsonObject>();
    
    // Dynamically add all preferences metadata and values to JSON doc
    for (size_t i = 0; i < prefCount; i++) {
        const RecorderPreferences::SettingMetadata& meta = metadata[i];
        
        // Add metadata
        JsonObject metaItem = metadataObj[meta.name].to<JsonObject>();
        metaItem["label"] = meta.label;
        metaItem["type"] = meta.type;
        
        // Add value
        if (strcmp(meta.type, "int") == 0) {
            valuesObj[meta.name] = preferences->getSettingInt(meta.name);
        } else if (strcmp(meta.type, "float") == 0) {
            valuesObj[meta.name] = preferences->getSettingFloat(meta.name);
        } else if (strcmp(meta.type, "bool") == 0) {
            valuesObj[meta.name] = preferences->getSettingBool(meta.name);
        } else {
            valuesObj[meta.name] = preferences->getSettingString(meta.name);
        }
    }
    
    String json;
    serializeJson(fullDoc, json);
    return json;
}

void RecorderServer::setSettingsFromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        log_e("JSON parsing error: %s", error.c_str());
        return;
    }
    
    size_t prefCount;
    const RecorderPreferences::SettingMetadata* metadata = RecorderPreferences::getSettingsMetadata(prefCount);
    
    // Iterate through metadata and update settings from JSON
    for (size_t i = 0; i < prefCount; i++) {
        const RecorderPreferences::SettingMetadata& meta = metadata[i];
        
        if (!doc[meta.name].isNull()) {
            if (strcmp(meta.type, "int") == 0) {
                preferences->setSetting(meta.name, doc[meta.name].as<int>());
            } else if (strcmp(meta.type, "float") == 0) {
                preferences->setSetting(meta.name, doc[meta.name].as<float>());
            } else if (strcmp(meta.type, "bool") == 0) {
                preferences->setSetting(meta.name, doc[meta.name].as<bool>());
            } else {
                preferences->setSetting(meta.name, doc[meta.name].as<String>());
            }
        }
    }
    
    log_i("Settings updated");
}

String RecorderServer::getHomeHtmlPage() {
    return String(R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SD Recorder - Home</title>)") 
    + String(RECORDER_CSS) + String(HOME_HTML_PAGE);
}


String RecorderServer::getSettingsHtmlPage() {
    return String(R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SD Recorder Settings</title>)")
     + String(RECORDER_CSS) + String(MODAL_CSS) + String(SETTINGS_HTML_PAGE);
}
