#include "recorderServer.h"
#include "styles.h"
#include "modalStyle.h"
#include "filesStyle.h"
#include <SD.h>

RecorderServer::RecorderServer(RecorderPreferences* preferences, SDCard* sdCard)
    : server(80), preferences(preferences), sdCard(sdCard), running(false),
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
    server.on("/files", HTTP_GET, [this]() { handleFiles(); });
    server.on("/download", HTTP_GET, [this]() { handleDownload(); });
    server.on("/apis/settings", HTTP_GET, [this]() { handleGetSettingsApi(); });
    server.on("/apis/settings", HTTP_POST, [this]() { handlePostSettingsApi(); });
    server.on("/apis/recording/start",  HTTP_POST, [this]() { handleStartRecordingApi(); });
    server.on("/apis/recording/stop",   HTTP_POST, [this]() { handleStopRecordingApi(); });
    server.on("/apis/recording/status", HTTP_GET,  [this]() { handleGetRecordingStatusApi(); });
    server.on("/apis/files/delete",     HTTP_POST, [this]() { handleDeleteFilesApi(); });
    server.on("/apis/system/restart",   HTTP_POST, [this]() { handleRestartApi(); });
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
    server.send(200, "text/html", getHtmlPage());
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
    
    DynamicJsonDocument doc(200);
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

void RecorderServer::handleFiles() {
    int count = sdCard->getFilesCount();

    String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SD Recorder - Files</title>
    )" + String(RECORDER_CSS) + String(MODAL_CSS) + String(FILES_CSS) + R"(
</head>
<body>
    <div class="container">
        <h1>SD Card Files</h1>
        <div class="settings-section">
)";

    if (count == 0) {
        html += "<p class=\"instruction-text\">No WAV files found on the SD card.</p>\n";
    } else {
        html += R"(
            <div class="select-all-container" id="selectAllContainer">
                <input type="checkbox" id="selectAllCheckbox" class="file-checkbox" onchange='toggleSelectAll()'>
                <label for="selectAllCheckbox" style="cursor: pointer; margin-left: 5px; flex-grow: 1;">Select All</label>
                <span class="selected-count" id="selectedCount">0 selected</span>
            </div>
            <div class="file-list-container" id="fileList">
)";
        
        for (int i = 0; i < count; i++) {
            const char* fullPath = sdCard->getFileName(i);
            if (!fullPath) continue;
            String fileName = String(fullPath);
            if (fileName.startsWith("/")) fileName = fileName.substring(1);

            // Get file size
            File f = SD.open(fullPath, FILE_READ);
            String sizeStr = "?";
            if (f) {
                size_t sz = f.size();
                f.close();
                if (sz >= 1024 * 1024) {
                    sizeStr = String(sz / (1024.0f * 1024.0f), 1) + " MB";
                } else if (sz >= 1024) {
                    sizeStr = String(sz / 1024.0f, 1) + " KB";
                } else {
                    sizeStr = String(sz) + " B";
                }
            }

            html += "<div class=\"file-item\">"
                    "<input type=\"checkbox\" class=\"file-checkbox file-select\" value=\"" + fileName + "\" onchange='updateSelectedCount()'>"
                    "<div class=\"file-info\">"
                    "<a href=\"/download?file=" + fileName + "\" class=\"file-link\">" + fileName + "</a>"
                    "<span class=\"file-size\">" + sizeStr + "</span>"
                    "</div>"
                    "</div>\n";
        }
        
        html += R"(
            </div>
            <div class="delete-section" id="deleteSection">
                <p id="deleteMessage">Ready to delete selected files.</p>
                <button class="btn-delete-files" onclick='deleteSelectedFiles()'>Delete Selected Files</button>
            </div>
)";
    }

    html += R"(
        </div>
        <button onclick="location.href='/'">Home</button>
    </div>

    <!-- Delete Confirmation Modal -->
    <div id="deleteModal" class="modal">
        <div class="modal-content">
            <h2>Delete Files?</h2>
            <p id="deleteConfirmMessage">Are you sure you want to delete the selected files? This action cannot be undone.</p>
            <div class="modal-buttons">
                <button class="btn-modal btn-cancel" onclick='cancelDelete()'>Cancel</button>
                <button class="btn-modal btn-confirm" onclick='confirmDelete()'>Delete</button>
            </div>
        </div>
    </div>

    <script>
        function getSelectedFiles() {
            const checkboxes = document.querySelectorAll('.file-select:checked');
            return Array.from(checkboxes).map(cb => cb.value);
        }

        function updateSelectedCount() {
            const selected = getSelectedFiles();
            const selectAllCheckbox = document.getElementById('selectAllCheckbox');
            const allCheckboxes = document.querySelectorAll('.file-select');
            const deleteSection = document.getElementById('deleteSection');
            const selectAllContainer = document.getElementById('selectAllContainer');
            const selectedCount = document.getElementById('selectedCount');

            selectAllCheckbox.checked = selected.length === allCheckboxes.length && allCheckboxes.length > 0;
            selectAllCheckbox.indeterminate = selected.length > 0 && selected.length < allCheckboxes.length;

            if (selected.length > 0) {
                deleteSection.classList.add('visible');
                selectAllContainer.classList.add('visible');
            } else {
                deleteSection.classList.remove('visible');
                selectAllContainer.classList.remove('visible');
            }

            selectedCount.textContent = selected.length + (selected.length === 1 ? ' file selected' : ' files selected');
        }

        function toggleSelectAll() {
            const selectAllCheckbox = document.getElementById('selectAllCheckbox');
            const fileCheckboxes = document.querySelectorAll('.file-select');
            fileCheckboxes.forEach(cb => {
                cb.checked = selectAllCheckbox.checked;
            });
            updateSelectedCount();
        }

        function deleteSelectedFiles() {
            const selected = getSelectedFiles();
            if (selected.length === 0) {
                alert('No files selected');
                return;
            }

            const message = 'Are you sure you want to delete ' + selected.length + ' file' + (selected.length === 1 ? '' : 's') + '?';
            document.getElementById('deleteConfirmMessage').textContent = message;
            document.getElementById('deleteModal').style.display = 'block';
        }

        function cancelDelete() {
            document.getElementById('deleteModal').style.display = 'none';
        }

        function confirmDelete() {
            document.getElementById('deleteModal').style.display = 'none';
            const selected = getSelectedFiles();

            fetch('/apis/files/delete', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ files: selected })
            })
            .then(response => response.json())
            .then(data => {
                if (data.deleted > 0) {
                    let message = 'Successfully deleted ' + data.deleted + ' file' + (data.deleted === 1 ? '' : 's');
                    if (data.failed > 0) {
                        message += '. Failed to delete ' + data.failed + ' file' + (data.failed === 1 ? '' : 's');
                    }
                    alert(message);
                    location.reload();
                } else {
                    alert('Failed to delete files.');
                }
            })
            .catch(error => {
                console.error('Error deleting files:', error);
                alert('Error deleting files');
            });
        }

        window.addEventListener('load', function() {
            updateSelectedCount();
        });

        window.onclick = function(event) {
            const modal = document.getElementById('deleteModal');
            if (event.target == modal) {
                modal.style.display = 'none';
            }
        }
    </script>
</body>
</html>
)";

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
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        log_e("JSON parsing error: %s", error.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("files") || !doc["files"].is<JsonArray>()) {
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
    StaticJsonDocument<512> doc;
    size_t prefCount;
    const RecorderPreferences::SettingMetadata* metadata = RecorderPreferences::getSettingsMetadata(prefCount);
    
    // Create a combined structure with metadata and values
    StaticJsonDocument<1024> fullDoc;
    JsonObject metadataObj = fullDoc.createNestedObject("metadata");
    JsonObject valuesObj = fullDoc.createNestedObject("values");
    
    // Dynamically add all preferences metadata and values to JSON doc
    for (size_t i = 0; i < prefCount; i++) {
        const RecorderPreferences::SettingMetadata& meta = metadata[i];
        
        // Add metadata
        JsonObject metaItem = metadataObj.createNestedObject(meta.name);
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
    StaticJsonDocument<512> doc;
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
        
        if (doc.containsKey(meta.name)) {
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

String RecorderServer::getHtmlPage() {
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SD Recorder - Home</title>
    )" + String(RECORDER_CSS) + R"(
      </head>
      <body>
      <div class="container">
      <h1>SD Recorder</h1>
      <h2 id="device-name">Loading...</h2>
        
        <div class="settings-section">
            <h2>Usage Instructions</h2>
            
            <div class="form-group">
                <label>📹 Short Button Press</label>
                <p class="instruction-text-bottom">Start or stop recording audio to the SD card</p>
            </div>
            
            <div class="form-group">
                <label>🔧 Long Button Press (2+ seconds)</label>
                <p class="instruction-text">Start or stop the WiFi acces point and server for remote control</p>
            </div>
        </div>
        
        <div class="settings-section">
            <h2>Recording Control</h2>
            <p class="device-status-text">
                <span class="recording-indicator" id="rec-dot"></span>
                Status: <strong id="recording-status">Loading...</strong>
            </p>
            <p id="current-filename" style="display:none; margin-top: 10px; font-size: 0.9em; color: #666;">
                File: <code id="filename-text"></code>
            </p>
            <button id="start-btn" class="btn-success" onclick='startRecording()'>&#9210; Start Recording</button>
            <button id="stop-btn" class="btn-danger"  onclick='stopRecording()'  style="display:none">&#9209; Stop Recording</button>
            <div id="rec-status" class="status"></div>
        </div>

        <div class="settings-section">           
            <button onclick="location.href='/files'">📁 File List</button>
            <button onclick="location.href='/settings'">⚙️ Settings</button>
        </div>
    </div>
    
    <script>
        window.addEventListener('load', function() {
            loadDeviceInfo();
            loadRecordingStatus();
            setInterval(loadRecordingStatus, 2000);
        });

        function loadDeviceInfo() {
            fetch('/apis/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('device-name').textContent = data.values.device_name || 'SDRecorder';
                })
                .catch(() => {
                    document.getElementById('device-name').textContent = 'SDRecorder';
                });
        }

        function loadRecordingStatus() {
            fetch('/apis/recording/status')
                .then(response => response.json())
                .then(data => {
                    const isRec = data.recording;
                    document.getElementById('recording-status').textContent = isRec ? 'Recording' : 'Idle';
                    document.getElementById('rec-dot').className = isRec ? 'recording-indicator active' : 'recording-indicator';
                    document.getElementById('start-btn').style.display = isRec ? 'none' : 'block';
                    document.getElementById('stop-btn').style.display = isRec ? 'block' : 'none';
                    
                    const filenameElement = document.getElementById('current-filename');
                    if (isRec && data.filename) {
                        document.getElementById('filename-text').textContent = data.filename;
                        filenameElement.style.display = 'block';
                    } else {
                        filenameElement.style.display = 'none';
                    }
                })
                .catch(() => {});
        }

        function startRecording() {
            fetch('/apis/recording/start', { method: 'POST' })
                .then(response => response.json())
                .then(() => loadRecordingStatus())
                .catch(() => {});
        }

        function stopRecording() {
            fetch('/apis/recording/stop', { method: 'POST' })
                .then(response => response.json())
                .then(() => loadRecordingStatus())
                .catch(() => {});
        }
    </script>
</body>
</html>
    )" ;
}


String RecorderServer::getSettingsHtmlPage() {
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SD Recorder Settings</title>
    )" + String(RECORDER_CSS) + String(MODAL_CSS) + R"(
</head>
<body>
    <div class="container">
        <h1>ESP32 Recorder Settings</h1>
        
        <div class="settings-section" id="settings-form">
            <!-- Form will be generated dynamically -->
        </div>
        
        <button onclick='saveSettings()'>Save Settings</button>
        <div id="status" class="status"></div>
        
        <div class="danger-section">
            <h3>⚠️ Device Control</h3>
            <button class="btn-danger-action" onclick='showRestartDialog()'>🔄 Restart Device</button>
        </div>
    </div>
    
    <!-- Restart Confirmation Dialog -->
    <div id="restartModal" class="modal">
        <div class="modal-content">
            <h2>Restart Device?</h2>
            <p>This will restart the ESP32 device. You may lose connection temporarily.</p>
            <div class="modal-buttons">
                <button class="btn-modal btn-cancel" onclick='cancelRestart()'>Cancel</button>
                <button class="btn-modal btn-confirm" onclick='confirmRestart()'>Restart</button>
            </div>
        </div>
    </div>
    
    <script>
        // Load settings on page load
        window.addEventListener('load', loadSettings);
        
        function detectType(typeStr) {
            if (typeStr === 'boolean' || typeStr === 'bool') {
                return 'boolean';
            }
            if (typeStr === 'int' || typeStr === 'integer') {
                return 'integer';
            }
            if (typeStr === 'float') {
                return 'float';
            }
            return 'string';
        }
        
        function createFormField(key, value, label, typeStr) {
            const type = detectType(typeStr);
            const formGroup = document.createElement('div');
            formGroup.className = 'form-group';
            
            if (type === 'boolean') {
                // Create checkbox
                const checkboxContainer = document.createElement('div');
                checkboxContainer.className = 'checkbox-container';
                
                const input = document.createElement('input');
                input.type = 'checkbox';
                input.id = key;
                input.checked = value;
                
                const labelEl = document.createElement('label');
                labelEl.htmlFor = key;
                labelEl.textContent = label;
                
                checkboxContainer.appendChild(input);
                checkboxContainer.appendChild(labelEl);
                formGroup.appendChild(checkboxContainer);
            } else {
                // Create text, number, or password input
                const labelEl = document.createElement('label');
                labelEl.htmlFor = key;
                labelEl.textContent = label;
                formGroup.appendChild(labelEl);
                
                const input = document.createElement('input');
                input.id = key;
                
                if (type === 'integer' || type === 'float') {
                    input.type = 'number';
                    if (type === 'float') {
                        input.step = '0.01';
                    }
                } else {
                    // Determine if password field based on key name
                    if (key.includes('password')) {
                        input.type = 'password';
                    } else {
                        input.type = 'text';
                    }
                }
                
                input.value = value;
                input.placeholder = label;
                formGroup.appendChild(input);
            }
            
            return formGroup;
        }
        
        function loadSettings() {
            fetch('/apis/settings')
                .then(response => response.json())
                .then(data => {
                    const formContainer = document.getElementById('settings-form');
                    formContainer.innerHTML = ''; // Clear existing fields
                    
                    // Iterate through all values and create form fields using metadata
                    const values = data.values || {};
                    const metadata = data.metadata || {};
                    
                    for (const key in values) {
                        if (values.hasOwnProperty(key)) {
                            const meta = metadata[key] || {};
                            const label = meta.label || key;
                            const type = meta.type || 'string';
                            const formField = createFormField(key, values[key], label, type);
                            formContainer.appendChild(formField);
                        }
                    }
                })
                .catch(error => {
                    console.error('Error loading settings:', error);
                    showStatus('Error loading settings', 'error');
                });
        }
        
        function saveSettings() {
            const formContainer = document.getElementById('settings-form');
            const inputs = formContainer.querySelectorAll('input');
            const settings = {};
            
            inputs.forEach(input => {
                const key = input.id;
                if (input.type === 'checkbox') {
                    settings[key] = input.checked;
                } else if (input.type === 'number') {
                    settings[key] = parseFloat(input.value);
                } else {
                    settings[key] = input.value;
                }
            });
            
            fetch('/apis/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(settings)
            })
            .then(response => response.json())
            .then(data => {
                showStatus('Settings saved successfully!', 'success');
            })
            .catch(error => {
                console.error('Error saving settings:', error);
                showStatus('Error saving settings', 'error');
            });
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + type;
            setTimeout(() => {
                statusDiv.className = 'status';
            }, 3000);
        }
        
        function showRestartDialog() {
            document.getElementById('restartModal').style.display = 'block';
        }
        
        function cancelRestart() {
            document.getElementById('restartModal').style.display = 'none';
        }
        
        function confirmRestart() {
            document.getElementById('restartModal').style.display = 'none';
            fetch('/apis/system/restart', {
                method: 'POST'
            })
            .then(response => {
                showStatus('Device restarting...', 'success');
            })
            .catch(error => {
                console.error('Error restarting device:', error);
                showStatus('Restart request sent', 'info');
            });
        }
        
        // Close modal when clicking outside of it
        window.onclick = function(event) {
            const modal = document.getElementById('restartModal');
            if (event.target == modal) {
                modal.style.display = 'none';
            }
        }
    </script>
</body>
</html>
    )" ;
}
