#ifndef HOME_HTML_PAGE_H
#define HOME_HTML_PAGE_H


const char HOME_HTML_PAGE[] = R"(
      </head>
      <body>
      <div class="container">
      <h1>SD Recorder</h1>
      <h2 id="device-name">Loading...</h2>
      <p id="device-time" style="text-align: center; color: #666; font-size: 0.95em; margin-top: -10px;">
        <span id="time-text">--:-- --</span>
      </p>
        
        <div class="settings-section">
            <h2>Usage Instructions</h2>
            
            <div class="form-group">
                <label>🎤 Short Button Press</label>
                <p class="instruction-text-bottom">Start or stop recording audio to the SD card</p>
            </div>
            
            <div class="form-group">
                <label>🛜 Long Button Press (2+ seconds)</label>
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
            <button onclick='goToFileList()'>📁 File List</button>
            <button onclick="location.href='/settings'">⚙️ Settings</button>
        </div>
    </div>
    
    <script>
        // Helper function to get browser's local time as Unix timestamp
        function getBrowserLocalTime() {
            const now = new Date();
            const utcTimestamp = Math.floor(now.getTime() / 1000);
            const timezoneOffset = now.getTimezoneOffset() * 60; // Convert minutes to seconds
            return utcTimestamp - timezoneOffset;
        }

        window.addEventListener('load', function() {
            loadDeviceInfo();
            loadRecordingStatus();
            loadDeviceTime();
            setInterval(loadRecordingStatus, 2000);
            setInterval(loadDeviceTime, 60000);
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

        function loadDeviceTime() {
            fetch('/apis/system/time')
                .then(response => response.json())
                .then(data => {
                    const timeEl = document.getElementById('time-text');
                    if (data.initialized) {
                        timeEl.textContent = data.time;
                    } else {
                        timeEl.textContent = 'Time not set - sync in settings';
                        timeEl.style.color = '#ffc107';
                    }
                })
                .catch(() => {
                    document.getElementById('time-text').textContent = 'Error loading time';
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
            const payload = JSON.stringify({ timestamp: getBrowserLocalTime() });
            
            fetch('/apis/recording/start', { 
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: payload
            })
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

        function goToFileList() {
            const payload = JSON.stringify({ timestamp: getBrowserLocalTime() });
            
            fetch('/apis/system/time', { 
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: payload
            })
                .then(() => {
                    location.href = '/files';
                })
                .catch(() => {
                    // Navigate anyway even if time sync fails
                    location.href = '/files';
                });
        }
    </script>
</body>
</html>
)";

#endif // HOME_HTML_PAGE_H
