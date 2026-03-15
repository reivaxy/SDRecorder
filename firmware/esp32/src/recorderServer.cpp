#include "recorderServer.h"
#include "styles.h"

RecorderServer::RecorderServer(RecorderPreferences* preferences, SDCard* sdCard)
    : server(80), preferences(preferences), sdCard(sdCard), running(false) {
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
    server.on("/apis/settings", HTTP_GET, [this]() { handleGetSettingsApi(); });
    server.on("/apis/settings", HTTP_POST, [this]() { handlePostSettingsApi(); });
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
    setSettingsFromJson(json);
    
    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void RecorderServer::handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"Not Found\"}");
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
        
        <div class="settings-section">
            <h2>Usage Instructions</h2>
            
            <div class="form-group">
                <label>📹 Short Button Press</label>
                <p class="instruction-text-bottom">Start or stop recording audio to the SD card</p>
            </div>
            
            <div class="form-group">
                <label>🔧 Long Button Press (2+ seconds)</label>
                <p class="instruction-text">Start or stop the WiFi server for remote control</p>
            </div>
        </div>
        
        <div class="settings-section">
            <h2>Device Status</h2>
            <p class="device-status-text">Device Name: <strong class="device-name" id="device-name">Loading...</strong></p>
            
            <button onclick="location.href='/settings'">⚙️ Settings</button>
            <button onclick="location.href='/files'">📁 File List</button>
        </div>
    </div>
    
    <script>
        // Load device name on page load
        window.addEventListener('load', loadDeviceInfo);
        
        function loadDeviceInfo() {
            fetch('/apis/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('device-name').textContent = data.values.device_name || 'SDRecorder';
                })
                .catch(error => {
                    console.error('Error loading device info:', error);
                    document.getElementById('device-name').textContent = 'SDRecorder';
                });
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
    )" + String(RECORDER_CSS) + R"(
</head>
<body>
    <div class="container">
        <h1>ESP32 Recorder Settings</h1>
        
        <div class="settings-section" id="settings-form">
            <!-- Form will be generated dynamically -->
        </div>
        
        <button onclick='saveSettings()'>Save Settings</button>
        <div id="status" class="status"></div>
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
    </script>
</body>
</html>
    )" ;
}
