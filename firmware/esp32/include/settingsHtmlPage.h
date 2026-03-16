#ifndef SETTINGS_HTML_PAGE_H
#define SETTINGS_HTML_PAGE_H


const char SETTINGS_HTML_PAGE[] = R"(
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
        
        <div class="danger-section">
            <h3>🕐 Time Synchronization</h3>
            <button onclick='syncTime()'>📡 Sync Time from Browser</button>
            <p id="timeStatus" style="font-size: 0.9em; margin-top: 10px; color: #666;"></p>
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
        
        function syncTime() {
            const timeStatusEl = document.getElementById('timeStatus');
            
            fetch('/apis/system/time', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ timestamp: getBrowserLocalTime() })
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    timeStatusEl.textContent = 'RTC synced! Device time: ' + data.time;
                    timeStatusEl.style.color = '#28a745';
                    showStatus('Time synchronized successfully!', 'success');
                } else {
                    timeStatusEl.textContent = 'Sync failed: ' + (data.error || 'Unknown error');
                    timeStatusEl.style.color = '#dc3545';
                    showStatus('Failed to sync time', 'error');
                }
            })
            .catch(error => {
                console.error('Error syncing time:', error);
                timeStatusEl.textContent = 'Error: ' + error.message;
                timeStatusEl.style.color = '#dc3545';
                showStatus('Error syncing time', 'error');
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
)";

#endif // SETTINGS_HTML_PAGE_H
