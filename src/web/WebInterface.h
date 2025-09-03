#pragma once

#include <Arduino.h>

namespace BattleAura {

// Embedded web interface as PROGMEM strings
const char MAIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BattleAura Controller</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #1a1a1a; 
            color: #fff; 
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
        }
        h1 { 
            text-align: center; 
            color: #4CAF50; 
            margin-bottom: 30px; 
        }
        .zone-card { 
            background: #2d2d2d; 
            border-radius: 8px; 
            padding: 20px; 
            margin: 10px 0; 
            border: 1px solid #444; 
        }
        .zone-name { 
            font-size: 18px; 
            font-weight: bold; 
            margin-bottom: 10px; 
            color: #4CAF50; 
        }
        .zone-info { 
            color: #aaa; 
            font-size: 14px; 
            margin-bottom: 15px; 
        }
        .brightness-control { 
            display: flex; 
            align-items: center; 
            gap: 10px; 
        }
        .brightness-slider { 
            flex: 1; 
            height: 6px; 
            border-radius: 3px; 
            background: #444; 
            outline: none; 
        }
        .brightness-value { 
            min-width: 40px; 
            text-align: right; 
            font-weight: bold; 
        }
        .status { 
            text-align: center; 
            padding: 10px; 
            margin: 20px 0; 
            border-radius: 4px; 
            background: #333; 
            border: 1px solid #555; 
        }
        .loading { 
            color: #ff9800; 
        }
        .error { 
            color: #f44336; 
            background: #2d1b1b; 
            border-color: #f44336; 
        }
        .success { 
            color: #4CAF50; 
            background: #1b2d1b; 
            border-color: #4CAF50; 
        }
        .footer { 
            text-align: center; 
            margin-top: 40px; 
            padding-top: 20px; 
            border-top: 1px solid #444; 
            color: #666; 
            font-size: 12px; 
        }
        .btn {
            padding: 10px 20px;
            background: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            margin: 5px;
            font-size: 14px;
        }
        .btn:hover { 
            background: #45a049; 
        }
        .btn:disabled { 
            background: #666; 
            cursor: not-allowed; 
        }
        .btn-success {
            background: #4CAF50;
        }
        .btn-danger {
            background: #f44336;
        }
        .btn-danger:hover {
            background: #da190b;
        }
        .section {
            margin: 30px 0;
            padding: 20px;
            background: #2d2d2d;
            border-radius: 8px;
            border: 1px solid #444;
        }
        .section h2 {
            color: #4CAF50;
            margin-top: 0;
            margin-bottom: 20px;
            border-bottom: 1px solid #444;
            padding-bottom: 10px;
        }
        .zone-form {
            margin-bottom: 20px;
        }
        .form-row {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            gap: 10px;
        }
        .form-row label {
            min-width: 120px;
            color: #ccc;
        }
        .form-row input, .form-row select {
            flex: 1;
            padding: 8px;
            background: #1a1a1a;
            border: 1px solid #555;
            color: #fff;
            border-radius: 4px;
        }
        .form-row input:focus, .form-row select:focus {
            border-color: #4CAF50;
            outline: none;
        }
        #brightnessValue {
            min-width: 40px;
            text-align: center;
            color: #4CAF50;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>BattleAura Controller</h1>
        
        <div id="status" class="status loading">
            Loading system...
        </div>
        
        <!-- Zone Configuration Section -->
        <div class="section">
            <h2>Zone Configuration</h2>
            <div class="zone-form">
                <h3>Add New Zone</h3>
                <div class="form-row">
                    <label>Name:</label>
                    <input type="text" id="zoneName" placeholder="e.g., Engine LED">
                </div>
                <div class="form-row">
                    <label>GPIO Pin:</label>
                    <input type="number" id="zoneGpio" min="2" max="21" placeholder="2-10, 20-21">
                </div>
                <div class="form-row">
                    <label>Type:</label>
                    <select id="zoneType">
                        <option value="PWM">PWM (Single LED)</option>
                        <option value="WS2812B">WS2812B (RGB Strip)</option>
                    </select>
                </div>
                <div class="form-row" id="ledCountRow" style="display:none;">
                    <label>LED Count:</label>
                    <input type="number" id="ledCount" min="1" max="100" value="5">
                </div>
                <div class="form-row">
                    <label>Group:</label>
                    <input type="text" id="zoneGroup" placeholder="e.g., Engines, Weapons" value="Default">
                </div>
                <div class="form-row">
                    <label>Max Brightness:</label>
                    <input type="range" id="zoneBrightness" min="1" max="255" value="255">
                    <span id="brightnessValue">255</span>
                </div>
                <button onclick="addZone()" class="btn btn-success">Add Zone</button>
                <button onclick="clearAllZones()" class="btn btn-danger">Clear All Zones</button>
            </div>
        </div>
        
        <!-- Current Zones Section -->
        <div class="section">
            <h2>Current Zones</h2>
            <div id="zones-container">
                <!-- Zones will be populated here -->
            </div>
        </div>
        
        <!-- Effect Control Section -->
        <div class="section">
            <h2>Effect Controls</h2>
            <div id="effects-container">
                <!-- Effects will be populated here -->
            </div>
        </div>
        
        <!-- Audio Control Section -->
        <div class="section">
            <h2>Audio Controls</h2>
            <div class="zone-card">
                <div class="zone-name">Audio Player</div>
                <div class="zone-info">
                    Status: <span id="audio-status">Unknown</span> | 
                    Track: <span id="current-track">None</span> |
                    Available: <span id="audio-available">Checking...</span>
                </div>
                <div style="margin-top: 15px;">
                    <div class="form-row">
                        <label>Track Number (1-9):</label>
                        <input type="number" id="track-number" min="1" max="9" value="1">
                        <input type="checkbox" id="loop-audio" style="margin-left: 10px;">
                        <label for="loop-audio" style="margin-left: 5px;">Loop</label>
                    </div>
                    <div class="form-row">
                        <label>Volume (0-30):</label>
                        <input type="range" id="audio-volume" min="0" max="30" value="15">
                        <span id="volume-value">15</span>
                    </div>
                    <div style="margin-top: 10px;">
                        <button onclick="playAudio()" class="btn btn-success" style="margin-right: 10px;">Play</button>
                        <button onclick="stopAudio()" class="btn btn-danger" style="margin-right: 10px;">Stop</button>
                        <button onclick="setVolume()" class="btn" style="margin-right: 10px;">Set Volume</button>
                        <button onclick="retryAudio()" class="btn" style="margin-right: 10px; background: #ff9800; color: white;">Retry Connection</button>
                        <button onclick="refreshAudioStatus()" class="btn">Refresh Status</button>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="zone-card">
            <div class="zone-name">Firmware Update</div>
            <div class="zone-info">Upload new firmware via OTA</div>
            <div style="margin-top: 15px;">
                <input type="file" id="firmware-file" accept=".bin" style="margin-bottom: 10px; width: 100%;">
                <button onclick="uploadFirmware()" style="width: 100%; padding: 10px; background: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer;">
                    Upload Firmware
                </button>
                <div id="upload-progress" style="margin-top: 10px; display: none;">
                    <div style="background: #444; border-radius: 4px; overflow: hidden;">
                        <div id="progress-bar" style="background: #4CAF50; height: 20px; width: 0%; transition: width 0.3s;"></div>
                    </div>
                    <span id="progress-text">0%</span>
                </div>
            </div>
        </div>
        
        <div class="footer">
            <span id="firmware-info">BattleAura Loading...</span><br>
            <span id="device-info"></span>
        </div>
    </div>

    <script>
        let zones = [];
        
        // Initialize on page load
        document.addEventListener('DOMContentLoaded', function() {
            loadZones();
            loadStatus();
            loadEffects();
            setupZoneForm();
        });
        
        // Load zones from server
        async function loadZones() {
            try {
                const response = await fetch('/api/zones');
                if (!response.ok) throw new Error('Failed to load zones');
                
                const data = await response.json();
                zones = data.zones || [];
                renderZones();
                updateStatus('success', `Loaded ${zones.length} zones`);
            } catch (error) {
                console.error('Error loading zones:', error);
                updateStatus('error', 'Failed to load zones: ' + error.message);
            }
        }
        
        // Load device status
        async function loadStatus() {
            try {
                const response = await fetch('/api/status');
                if (!response.ok) throw new Error('Failed to load status');
                
                const data = await response.json();
                document.getElementById('firmware-info').textContent = 
                    `BattleAura ${data.firmwareVersion}`;
                document.getElementById('device-info').textContent = 
                    `${data.deviceName} | ${data.ip} | Uptime: ${formatUptime(data.uptime)}`;
            } catch (error) {
                console.error('Error loading status:', error);
            }
        }
        
        // Render zones in UI
        function renderZones() {
            const container = document.getElementById('zones-container');
            
            if (zones.length === 0) {
                container.innerHTML = '<div class="status">No zones configured</div>';
                return;
            }
            
            container.innerHTML = zones.map(zone => `
                <div class="zone-card">
                    <div class="zone-name">${zone.name}</div>
                    <div class="zone-info">
                        GPIO ${zone.gpio} | ${zone.type} | Group: ${zone.groupName} | Max: ${zone.brightness}
                    </div>
                    <div class="brightness-control">
                        <span>Brightness:</span>
                        <input type="range" 
                               class="brightness-slider" 
                               min="0" 
                               max="${zone.brightness}" 
                               value="${zone.currentBrightness || 0}"
                               onchange="setBrightness(${zone.id}, this.value)"
                               oninput="updateBrightnessDisplay(${zone.id}, this.value)">
                        <span class="brightness-value" id="brightness-${zone.id}">
                            ${zone.currentBrightness || 0}
                        </span>
                    </div>
                </div>
            `).join('');
        }
        
        // Update brightness display
        function updateBrightnessDisplay(zoneId, value) {
            document.getElementById(`brightness-${zoneId}`).textContent = value;
        }
        
        // Set brightness on server
        async function setBrightness(zoneId, brightness) {
            try {
                const response = await fetch('/api/brightness', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ zoneId, brightness: parseInt(brightness) })
                });
                
                if (!response.ok) throw new Error('Failed to set brightness');
                
                // Update local zone data
                const zone = zones.find(z => z.id === zoneId);
                if (zone) zone.currentBrightness = parseInt(brightness);
                
                updateStatus('success', `Zone ${zoneId} brightness set to ${brightness}`);
            } catch (error) {
                console.error('Error setting brightness:', error);
                updateStatus('error', 'Failed to set brightness: ' + error.message);
                loadZones(); // Reload to reset UI
            }
        }
        
        // Update status message
        function updateStatus(type, message) {
            const statusEl = document.getElementById('status');
            statusEl.className = `status ${type}`;
            statusEl.textContent = message;
            
            // Clear success messages after 3 seconds
            if (type === 'success') {
                setTimeout(() => {
                    statusEl.className = 'status';
                    statusEl.textContent = 'Ready';
                }, 3000);
            }
        }
        
        // Format uptime
        function formatUptime(ms) {
            const seconds = Math.floor(ms / 1000);
            const minutes = Math.floor(seconds / 60);
            const hours = Math.floor(minutes / 60);
            
            if (hours > 0) return `${hours}h ${minutes % 60}m`;
            if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
            return `${seconds}s`;
        }
        
        // Upload firmware
        async function uploadFirmware() {
            const fileInput = document.getElementById('firmware-file');
            const file = fileInput.files[0];
            
            if (!file) {
                updateStatus('error', 'Please select a firmware file');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                updateStatus('error', 'Please select a .bin file');
                return;
            }
            
            const progressDiv = document.getElementById('upload-progress');
            const progressBar = document.getElementById('progress-bar');
            const progressText = document.getElementById('progress-text');
            
            try {
                updateStatus('loading', 'Starting firmware upload...');
                progressDiv.style.display = 'block';
                
                const formData = new FormData();
                formData.append('firmware', file);
                
                const response = await fetch('/update', {
                    method: 'POST',
                    body: formData
                });
                
                if (!response.ok) throw new Error(`Upload failed: ${response.statusText}`);
                
                updateStatus('success', 'Firmware uploaded successfully! Device will restart...');
                progressBar.style.width = '100%';
                progressText.textContent = '100%';
                
                // Reset form after delay
                setTimeout(() => {
                    fileInput.value = '';
                    progressDiv.style.display = 'none';
                    progressBar.style.width = '0%';
                    progressText.textContent = '0%';
                }, 3000);
                
            } catch (error) {
                console.error('Upload error:', error);
                updateStatus('error', 'Upload failed: ' + error.message);
                progressDiv.style.display = 'none';
            }
        }
        
        // Zone configuration functions
        function setupZoneForm() {
            // Setup zone type change handler
            document.getElementById('zoneType').addEventListener('change', function() {
                const ledCountRow = document.getElementById('ledCountRow');
                ledCountRow.style.display = this.value === 'WS2812B' ? 'flex' : 'none';
            });
            
            // Setup brightness slider
            document.getElementById('zoneBrightness').addEventListener('input', function() {
                document.getElementById('brightnessValue').textContent = this.value;
            });
        }
        
        async function addZone() {
            const name = document.getElementById('zoneName').value.trim();
            const gpio = parseInt(document.getElementById('zoneGpio').value);
            const type = document.getElementById('zoneType').value;
            const ledCount = parseInt(document.getElementById('ledCount').value) || 1;
            const groupName = document.getElementById('zoneGroup').value.trim() || 'Default';
            const brightness = parseInt(document.getElementById('zoneBrightness').value);
            
            // Validate inputs
            if (!name) {
                updateStatus('error', 'Zone name is required');
                return;
            }
            
            if (!gpio || gpio < 2 || gpio > 21) {
                updateStatus('error', 'Valid GPIO pin (2-21) is required');
                return;
            }
            
            try {
                updateStatus('loading', 'Adding zone...');
                
                const response = await fetch('/api/zones', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        name, gpio, type, ledCount, groupName, brightness
                    })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    clearZoneForm();
                    loadZones(); // Reload zones
                    loadEffects(); // Reload effects (might have changed)
                } else {
                    updateStatus('error', result.error || 'Failed to add zone');
                }
                
            } catch (error) {
                console.error('Error adding zone:', error);
                updateStatus('error', 'Failed to add zone: ' + error.message);
            }
        }
        
        function clearZoneForm() {
            document.getElementById('zoneName').value = '';
            document.getElementById('zoneGpio').value = '';
            document.getElementById('zoneType').value = 'PWM';
            document.getElementById('ledCount').value = '5';
            document.getElementById('zoneGroup').value = 'Default';
            document.getElementById('zoneBrightness').value = '255';
            document.getElementById('brightnessValue').textContent = '255';
            document.getElementById('ledCountRow').style.display = 'none';
        }
        
        async function clearAllZones() {
            if (!confirm('Are you sure you want to remove all zones? This cannot be undone.')) {
                return;
            }
            
            try {
                updateStatus('loading', 'Clearing all zones...');
                
                const response = await fetch('/api/zones/clear', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    loadZones(); // Reload zones
                    loadEffects(); // Reload effects
                } else {
                    updateStatus('error', result.error || 'Failed to clear zones');
                }
                
            } catch (error) {
                console.error('Error clearing zones:', error);
                updateStatus('error', 'Failed to clear zones: ' + error.message);
            }
        }
        
        // Effect management functions
        async function loadEffects() {
            try {
                const response = await fetch('/api/effects');
                if (!response.ok) throw new Error('Failed to load effects');
                
                const data = await response.json();
                renderEffects(data.effects || []);
            } catch (error) {
                console.error('Error loading effects:', error);
            }
        }
        
        function renderEffects(effects) {
            const container = document.getElementById('effects-container');
            
            if (effects.length === 0) {
                container.innerHTML = '<div class="status">No effects available</div>';
                return;
            }
            
            container.innerHTML = effects.map(effect => `
                <div class="zone-card">
                    <div class="zone-name">${effect.name}</div>
                    <div class="zone-info">
                        Status: ${effect.enabled ? 'Running' : 'Stopped'}
                    </div>
                    <button onclick="triggerEffect('${effect.name}', 0)" class="btn">
                        ${effect.enabled ? 'Restart' : 'Start'} Continuous
                    </button>
                    <button onclick="triggerEffect('${effect.name}', 2000)" class="btn">
                        Trigger 2s
                    </button>
                    <button onclick="triggerEffect('${effect.name}', 5000)" class="btn">
                        Trigger 5s
                    </button>
                </div>
            `).join('');
        }
        
        async function triggerEffect(effectName, duration) {
            try {
                updateStatus('loading', `Triggering ${effectName}...`);
                
                const response = await fetch('/api/effects/trigger', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ effectName, duration })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    setTimeout(() => loadEffects(), 500); // Reload effects after delay
                } else {
                    updateStatus('error', result.error || 'Failed to trigger effect');
                }
                
            } catch (error) {
                console.error('Error triggering effect:', error);
                updateStatus('error', 'Failed to trigger effect: ' + error.message);
            }
        }
        
        // Audio control functions
        async function playAudio() {
            const trackNumber = parseInt(document.getElementById('track-number').value);
            const loop = document.getElementById('loop-audio').checked;
            
            try {
                updateStatus('loading', `Playing track ${trackNumber}...`);
                
                const response = await fetch('/api/audio/play', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ trackNumber, loop })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    setTimeout(() => refreshAudioStatus(), 500);
                } else {
                    updateStatus('error', result.error || 'Failed to play audio');
                }
                
            } catch (error) {
                console.error('Error playing audio:', error);
                updateStatus('error', 'Failed to play audio: ' + error.message);
            }
        }
        
        async function stopAudio() {
            try {
                updateStatus('loading', 'Stopping audio...');
                
                const response = await fetch('/api/audio/stop', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    setTimeout(() => refreshAudioStatus(), 500);
                } else {
                    updateStatus('error', result.error || 'Failed to stop audio');
                }
                
            } catch (error) {
                console.error('Error stopping audio:', error);
                updateStatus('error', 'Failed to stop audio: ' + error.message);
            }
        }
        
        async function setVolume() {
            const volume = parseInt(document.getElementById('audio-volume').value);
            
            try {
                updateStatus('loading', `Setting volume to ${volume}...`);
                
                const response = await fetch('/api/audio/volume', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ volume })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    setTimeout(() => refreshAudioStatus(), 500);
                } else {
                    updateStatus('error', result.error || 'Failed to set volume');
                }
                
            } catch (error) {
                console.error('Error setting volume:', error);
                updateStatus('error', 'Failed to set volume: ' + error.message);
            }
        }
        
        async function refreshAudioStatus() {
            try {
                const response = await fetch('/api/audio/status');
                if (!response.ok) throw new Error('Failed to get audio status');
                
                const data = await response.json();
                
                document.getElementById('audio-status').textContent = data.status || 'Unknown';
                document.getElementById('current-track').textContent = 
                    data.currentTrack > 0 ? data.currentTrack : 'None';
                document.getElementById('audio-available').textContent = 
                    data.available ? 'Yes' : 'No';
                
                // Update volume slider
                document.getElementById('audio-volume').value = data.volume || 15;
                document.getElementById('volume-value').textContent = data.volume || 15;
                
            } catch (error) {
                console.error('Error getting audio status:', error);
                document.getElementById('audio-status').textContent = 'Error';
                document.getElementById('audio-available').textContent = 'Error';
            }
        }
        
        async function retryAudio() {
            try {
                updateStatus('loading', 'Retrying audio connection...');
                
                const response = await fetch('/api/audio/retry', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    setTimeout(() => refreshAudioStatus(), 1000);
                } else {
                    updateStatus('error', result.error || 'Failed to retry audio connection');
                }
                
            } catch (error) {
                console.error('Error retrying audio connection:', error);
                updateStatus('error', 'Failed to retry audio connection: ' + error.message);
            }
        }
        
        // Update volume display when slider changes
        document.addEventListener('DOMContentLoaded', function() {
            const volumeSlider = document.getElementById('audio-volume');
            if (volumeSlider) {
                volumeSlider.addEventListener('input', function() {
                    document.getElementById('volume-value').textContent = this.value;
                });
            }
            
            // Load initial audio status
            refreshAudioStatus();
        });
    </script>
</body>
</html>
)rawliteral";

} // namespace BattleAura