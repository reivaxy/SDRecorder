#include "recorderServer.h"

RecorderServer::RecorderServer(Preferences* settings, SDCard* sdCard)
    : server(80), settings(settings), sdCard(sdCard), running(false) {
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
    String ssid = settings->getString("ap_ssid", "SDRecorder");
    String password = settings->getString("ap_password", "12345678");
    log_i("Starting AP with SSID: %s", ssid.c_str());
    
    if (!WiFi.softAP(ssid, password)) {
        log_e("Failed to start AP");
        return;
    }
    
    log_i("AP started: %s", ssid.c_str());
    log_i("IP address: %s", WiFi.softAPIP().toString().c_str());
    
    // Register request handlers
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/settings", HTTP_POST, [this]() { handlePostSettings(); });
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

void RecorderServer::handleGetSettings() {
    server.send(200, "application/json", getSettingsJson());
}

void RecorderServer::handlePostSettings() {
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
    
    doc["ap_ssid"] = settings->getString("ap_ssid", "SDRecorder");
    doc["ap_password"] = settings->getString("ap_password", "12345678");
    doc["sample_rate"] = settings->getInt("sample_rate", SAMPLE_RATE);
    doc["device_name"] = settings->getString("device_name", "SDRecorder");
    
    String json;
    serializeJson(doc, json);
    return json;
}

void RecorderServer::setSettingsFromJson(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        log_e("JSON parsing error: %s", error.c_str());
        return;
    }
    
    if (doc.containsKey("ap_ssid")) {
        settings->putString("ap_ssid", doc["ap_ssid"].as<String>());
    }
    if (doc.containsKey("ap_password")) {
        settings->putString("ap_password", doc["ap_password"].as<String>());
    }
    if (doc.containsKey("device_name")) {
        settings->putString("device_name", doc["device_name"].as<String>());
    }
    if (doc.containsKey("sample_rate")) {
        settings->putInt("sample_rate", doc["sample_rate"].as<int>());
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
    <title>ESP32 Recorder</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 600px;
            width: 100%;
            padding: 40px;
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
            text-align: center;
        }
        .settings-section {
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-weight: 500;
        }
        input, select {
            width: 100%;
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            width: 100%;
            padding: 12px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        button:active {
            transform: translateY(0);
        }
        .status {
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
            display: none;
            text-align: center;
            font-weight: 600;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            display: block;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            display: block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Recorder Settings</h1>
        
        <div class="settings-section">
            <div class="form-group">
                <label for="device_name">Device Name</label>
                <input type="text" id="device_name" placeholder="ESP32-Recorder">
            </div>
            
            <div class="form-group">
                <label for="ap_ssid">WiFi Network Name (SSID)</label>
                <input type="text" id="ap_ssid" placeholder="ESP32-Recorder">
            </div>
            
            <div class="form-group">
                <label for="ap_password">WiFi Password</label>
                <input type="password" id="ap_password" placeholder="12345678">
            </div>
            
            <div class="form-group">
                <label for="sample_rate">Sample Rate</label>
                <select id="sample_rate">
                    <option value="16000">16000 Hz</option>
                    <option value="32000">32000 Hz</option>
                    <option value="48000">48000 Hz (Default)</option>
                </select>
            </div>
            
            <button onclick='saveSettings()'>Save Settings</button>
            <div id="status" class="status"></div>
        </div>
    </div>
    
    <script>
        // Load settings on page load
        window.addEventListener('load', loadSettings);
        
        function loadSettings() {
            fetch('/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('device_name').value = data.device_name || 'ESP32-Recorder';
                    document.getElementById('ap_ssid').value = data.ap_ssid || 'ESP32-Recorder';
                    document.getElementById('ap_password').value = data.ap_password || '12345678';
                    document.getElementById('sample_rate').value = data.sample_rate || 48000;
                })
                .catch(error => {
                    console.error('Error loading settings:', error);
                    showStatus('Error loading settings', 'error');
                });
        }
        
        function saveSettings() {
            const settings = {
                device_name: document.getElementById('device_name').value,
                ap_ssid: document.getElementById('ap_ssid').value,
                ap_password: document.getElementById('ap_password').value,
                sample_rate: parseInt(document.getElementById('sample_rate').value)
            };
            
            fetch('/settings', {
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
    )";
}
