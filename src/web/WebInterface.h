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
        .nav-tabs {
            display: flex;
            margin-bottom: 20px;
            border-bottom: 1px solid #444;
        }
        .tab-btn {
            padding: 10px 20px;
            background: #333;
            color: #ccc;
            border: none;
            border-bottom: 2px solid transparent;
            cursor: pointer;
            margin-right: 5px;
        }
        .tab-btn.active {
            color: #4CAF50;
            border-bottom-color: #4CAF50;
        }
        .tab-btn:hover {
            background: #444;
        }
        .config-tab {
            display: none;
        }
        .config-tab.active {
            display: block;
        }
        .form-row {
            display: flex;
            align-items: center;
            margin: 10px 0;
            gap: 10px;
        }
        .form-row label {
            min-width: 100px;
        }
        .form-row input, .form-row select {
            flex: 1;
            padding: 5px;
            border: 1px solid #555;
            background: #333;
            color: white;
            border-radius: 3px;
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
            max-width: 200px;
            padding: 8px;
            background: #1a1a1a;
            border: 1px solid #555;
            color: #fff;
            border-radius: 4px;
            overflow: hidden;
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
        
        <!-- Effect Control Section - Primary Use Case -->
        <div class="section">
            <h2>Effect Controls</h2>
            <p>Quick access to trigger effects on your configured zones</p>
            <div id="effects-container">
                <!-- Effects will be populated here -->
            </div>
        </div>
        
        <!-- Global Controls -->
        <div class="section">
            <h2>Global Controls</h2>
            <div class="zone-card">
                <div class="zone-name">Master Controls</div>
                <div style="margin-top: 15px;">
                    <div class="form-row">
                        <label for="global-brightness">Global Brightness (0-255):</label>
                        <input type="range" id="global-brightness" min="0" max="255" value="255" oninput="setGlobalBrightness(this.value)">
                        <span id="global-brightness-value">255</span>
                    </div>
                    <div style="margin-top: 10px;">
                        <button onclick="stopAllEffects()" class="btn btn-danger" style="margin-right: 10px;">Stop All Effects</button>
                        <button onclick="setAllZonesBrightness(0)" class="btn" style="margin-right: 10px; background: #666;">Lights Off</button>
                        <button onclick="setAllZonesBrightness(255)" class="btn" style="background: #ff9800;">Lights On</button>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- Quick Audio Controls -->
        <div class="section">
            <h2>Audio Player</h2>
            <div class="zone-card">
                <div class="zone-name">Manual Audio Control</div>
                <div class="zone-info">
                    Status: <span id="audio-status">Unknown</span> | 
                    Track: <span id="current-track">None</span>
                </div>
                <div style="margin-top: 15px;">
                    <div class="form-row">
                        <label for="track-number">Track (1-9):</label>
                        <input type="number" id="track-number" min="1" max="9" value="1" style="width: 60px;">
                        <input type="checkbox" id="loop-audio" style="margin-left: 10px;">
                        <label for="loop-audio" style="margin-left: 5px;">Loop</label>
                        <button onclick="playAudio()" class="btn btn-success" style="margin-left: 10px;">Play</button>
                        <button onclick="stopAudio()" class="btn btn-danger">Stop</button>
                    </div>
                    <div class="form-row">
                        <label for="audio-volume">Volume:</label>
                        <input type="range" id="audio-volume" min="0" max="30" value="15" oninput="setVolume(this.value)">
                        <span id="volume-value">15</span>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- Configuration Section -->
        <div class="section" id="config-section" style="display: none;">
            <h2>Configuration</h2>
            <div class="nav-tabs">
                <button class="tab-btn active" onclick="showConfigTab('zones')">Zones</button>
                <button class="tab-btn" onclick="showConfigTab('effects')">Effects & Audio</button>
                <button class="tab-btn" onclick="showConfigTab('device')">Device</button>
                <button class="tab-btn" onclick="showConfigTab('system')">System</button>
            </div>
            
            <!-- Zones Config Tab -->
            <div id="config-zones" class="config-tab active">
                <h3>Zone Management</h3>
                <div class="zone-form">
                    <div class="form-row">
                        <label for="newZoneName">Name:</label>
                        <input type="text" id="newZoneName" placeholder="e.g., Engine LED">
                    </div>
                    <div class="form-row">
                        <label for="newZoneGpio">GPIO Pin:</label>
                        <input type="number" id="newZoneGpio" min="2" max="21" placeholder="2-10, 20-21">
                    </div>
                    <div class="form-row">
                        <label for="newZoneType">Type:</label>
                        <select id="newZoneType">
                            <option value="PWM">PWM (Single LED)</option>
                            <option value="WS2812B">WS2812B (RGB Strip)</option>
                        </select>
                    </div>
                    <div class="form-row" id="newLedCountRow" style="display:none;">
                        <label for="newLedCount">LED Count:</label>
                        <input type="number" id="newLedCount" min="1" max="100" value="5">
                    </div>
                    <div class="form-row">
                        <label for="newZoneGroup">Group:</label>
                        <input type="text" id="newZoneGroup" placeholder="e.g., Engines, Weapons" value="Default">
                    </div>
                    <button onclick="addNewZone()" class="btn btn-success">Add Zone</button>
                    <button onclick="clearAllZones()" class="btn btn-danger">Clear All Zones</button>
                </div>
                <div id="configured-zones-list">
                    <h4>Configured Zones</h4>
                    <div id="zones-config-list"></div>
                </div>
            </div>
            
            <!-- Effects & Audio Config Tab -->
            <div id="config-effects" class="config-tab">
                <h3>Audio Tracks</h3>
                <p>Configure audio files for effects. Files must be named 0001.mp3, 0002.mp3, etc. on SD card.</p>
                <div class="form-row">
                    <label>File Number:</label>
                    <input type="number" id="trackNumber" min="1" max="999" value="1">
                    <label>Description:</label>
                    <input type="text" id="trackDescription" placeholder="Track description">
                    <input type="checkbox" id="trackLoop"> <label for="trackLoop">Loop</label>
                    <button onclick="addAudioTrack()" class="btn">Add Track</button>
                </div>
                <div id="audio-tracks-list"></div>
                
                <h3 style="margin-top: 30px;">Effect Configuration</h3>
                <p>Configure which effects apply to which groups and their audio associations.</p>
                <div class="form-row">
                    <label>Effect:</label>
                    <select id="effectName">
                        <!-- Will be populated from available effects -->
                    </select>
                    <label>Group:</label>
                    <input type="text" id="effectGroup" placeholder="e.g., Weapons, Engines">
                    <label>Audio Track:</label>
                    <select id="effectAudio">
                        <option value="0">None</option>
                        <!-- Will be populated from audio tracks -->
                    </select>
                    <button onclick="addEffectConfig()" class="btn">Configure Effect</button>
                </div>
                <div id="effect-configs-list"></div>
            </div>
            
            <!-- Device Config Tab -->
            <div id="config-device" class="config-tab">
                <h3>Device Information</h3>
                <div class="zone-card" style="margin-bottom: 20px;">
                    <div class="zone-name">Current Status</div>
                    <div class="zone-info">
                        WiFi: <span id="wifi-status">Checking...</span> | 
                        Network: <span id="wifi-ssid">None</span> |
                        IP: <span id="wifi-ip">Unknown</span> |
                        Hostname: <span id="device-hostname">Unknown</span>
                    </div>
                    <div style="margin-top: 10px;">
                        <button onclick="refreshWiFiStatus()" class="btn">Refresh Status</button>
                    </div>
                </div>
                
                <h3>WiFi Configuration</h3>
                <div class="form-row">
                    <label for="deviceName">Device Name:</label>
                    <input type="text" id="deviceName" placeholder="e.g., BattleTank" maxlength="32">
                    <small style="color: #666; margin-left: 10px;">Used for hostname (e.g., battletank.local)</small>
                </div>
                <div class="form-row">
                    <label for="wifiNetwork">Network Name (SSID):</label>
                    <input type="text" id="wifiNetwork" placeholder="e.g., MyWiFi" maxlength="32">
                </div>
                <form>
                    <input type="text" name="username" autocomplete="username" style="display: none;">
                    <div class="form-row">
                        <label for="wifiPassword">Password:</label>
                        <input type="password" id="wifiPassword" name="password" placeholder="WiFi Password" maxlength="64" autocomplete="new-password">
                        <input type="checkbox" id="showPassword" style="margin-left: 10px;">
                        <label for="showPassword" style="margin-left: 5px;">Show</label>
                    </div>
                </form>
                <div style="margin-top: 10px;">
                    <button onclick="saveWiFiConfig()" class="btn btn-success" style="margin-right: 10px;">Save & Connect</button>
                    <button onclick="clearWiFiConfig()" class="btn btn-danger">Clear WiFi</button>
                </div>
                
                <h3 style="margin-top: 30px;">Audio Settings</h3>
                <div class="form-row">
                    <label for="audioEnabled">Audio Enabled:</label>
                    <input type="checkbox" id="audioEnabled" checked>
                </div>
                <button onclick="saveDeviceConfig()" class="btn btn-success">Save Device Settings</button>
            </div>
            
            <!-- System Config Tab -->
            <div id="config-system" class="config-tab">
                <h3>System Management</h3>
                <div style="margin: 20px 0;">
                    <button onclick="restartDevice()" class="btn btn-warning">Restart Device</button>
                    <button onclick="factoryReset()" class="btn btn-danger">Factory Reset</button>
                </div>
                
                <h3>Firmware Update</h3>
                <p>Upload new firmware via OTA</p>
                <div style="margin-top: 15px;">
                    <input type="file" id="firmwareFile" accept=".bin" style="margin-bottom: 10px; width: 100%;">
                    <button onclick="uploadFirmware()" class="btn btn-success" style="width: 100%; padding: 10px;">
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
        </div>
        
        <div style="text-align: center; margin: 30px 0;">
            <button onclick="toggleConfig()" class="btn" id="config-toggle">Show Configuration</button>
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
            setupNewZoneForm();
            setTimeout(() => {
                loadAudioTracks();
                loadEffectConfigs();
                loadAvailableEffects();
            }, 1000);
        });
        
        // Configuration functions
        function toggleConfig() {
            const section = document.getElementById('config-section');
            const btn = document.getElementById('config-toggle');
            
            if (section && btn) {
                if (section.style.display === 'none' || section.style.display === '') {
                    section.style.display = 'block';
                    btn.textContent = 'Hide Configuration';
                } else {
                    section.style.display = 'none';
                    btn.textContent = 'Show Configuration';
                }
            }
        }
        
        function showConfigTab(tabName) {
            // Hide all tabs
            document.querySelectorAll('.config-tab').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab-btn').forEach(btn => {
                btn.classList.remove('active');
            });
            
            // Show selected tab
            const targetTab = document.getElementById('config-' + tabName);
            if (targetTab && event && event.target) {
                targetTab.classList.add('active');
                event.target.classList.add('active');
            }
        }
        
        async function addNewZone() {
            const name = document.getElementById('newZoneName')?.value.trim();
            const gpio = parseInt(document.getElementById('newZoneGpio')?.value);
            const type = document.getElementById('newZoneType')?.value;
            const group = document.getElementById('newZoneGroup')?.value.trim();
            
            if (!name || !group || !gpio) {
                updateStatus('error', 'Please fill all zone fields');
                return;
            }
            
            try {
                const response = await fetch('/api/zones', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        name: name,
                        gpio: gpio,
                        type: type,
                        groupName: group,
                        brightness: 255,
                        ledCount: type === 'WS2812B' ? parseInt(document.getElementById('newLedCount')?.value) || 5 : 1,
                        enabled: true
                    })
                });
                
                const result = await response.json();
                if (response.ok) {
                    updateStatus('success', 'Zone added successfully');
                    const nameField = document.getElementById('newZoneName');
                    const groupField = document.getElementById('newZoneGroup');
                    if (nameField) nameField.value = '';
                    if (groupField) groupField.value = 'Default';
                    loadZones();
                } else {
                    updateStatus('error', result.error || 'Failed to add zone');
                }
            } catch (error) {
                updateStatus('error', 'Failed to add zone: ' + error.message);
            }
        }
        
        async function addAudioTrack() {
            const fileNumber = parseInt(document.getElementById('trackNumber')?.value);
            const description = document.getElementById('trackDescription')?.value.trim();
            const isLoop = document.getElementById('trackLoop')?.checked;
            
            if (!description) {
                updateStatus('error', 'Please enter track description');
                return;
            }
            
            try {
                const response = await fetch('/api/audio/tracks', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        fileNumber: fileNumber,
                        description: description,
                        isLoop: isLoop,
                        duration: 0
                    })
                });
                
                const result = await response.json();
                if (response.ok) {
                    updateStatus('success', 'Audio track added');
                    const descField = document.getElementById('trackDescription');
                    if (descField) descField.value = '';
                    loadAudioTracks();
                } else {
                    updateStatus('error', result.error || 'Failed to add audio track');
                }
            } catch (error) {
                updateStatus('error', 'Failed to add audio track: ' + error.message);
            }
        }
        
        async function loadAudioTracks() {
            try {
                const response = await fetch('/api/audio/tracks');
                if (!response.ok) return;
                
                const data = await response.json();
                const container = document.getElementById('audio-tracks-list');
                
                if (!container) return; // Element doesn't exist yet
                
                if (!data.tracks || data.tracks.length === 0) {
                    container.innerHTML = '<p>No audio tracks configured</p>';
                    return;
                }
                
                container.innerHTML = data.tracks.map(track => 
                    `<div style="padding: 10px; background: #333; margin: 5px 0; border-radius: 3px; border: 1px solid #555;">
                        <strong>Track ${track.fileNumber}: ${track.description}</strong> ${track.isLoop ? '(Loop)' : '(One-shot)'}
                        <div style="margin-top: 5px;">
                            <button onclick="testAudioTrack(${track.fileNumber})" class="btn btn-warning" style="padding: 5px 10px; margin-right: 5px;">Test</button>
                            <button onclick="removeAudioTrack(${track.fileNumber})" class="btn btn-danger" style="padding: 5px 10px;">Remove</button>
                        </div>
                    </div>`
                ).join('');
                
                // Update effect audio select options
                populateAudioSelects(data.tracks);
            } catch (error) {
                console.error('Error loading audio tracks:', error);
            }
        }
        
        async function testAudioTrack(fileNumber) {
            try {
                updateStatus('loading', 'Testing audio track...');
                
                const response = await fetch('/api/audio/play', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        trackNumber: fileNumber,
                        loop: false
                    })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', 'Playing audio track');
                } else {
                    updateStatus('error', result.error || 'Failed to play audio track');
                }
                
            } catch (error) {
                console.error('Error testing audio track:', error);
                updateStatus('error', 'Failed to test audio track: ' + error.message);
            }
        }
        
        function populateAudioSelects(tracks) {
            const select = document.getElementById('effectAudio');
            if (!select) return;
            
            select.innerHTML = '<option value="0">None</option>' + 
                tracks.map(track => 
                    `<option value="${track.fileNumber}">Track ${track.fileNumber}: ${track.description}</option>`
                ).join('');
        }
        
        async function addEffectConfig() {
            const effectName = document.getElementById('effectName')?.value;
            const group = document.getElementById('effectGroup')?.value.trim();
            const audioFile = document.getElementById('effectAudio')?.value;
            
            if (!effectName || !group) {
                updateStatus('error', 'Please select effect and enter group name');
                return;
            }
            
            try {
                updateStatus('loading', 'Configuring effect...');
                
                const response = await fetch('/api/effects/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        name: effectName,
                        groups: [group],
                        audioFile: parseInt(audioFile) || 0
                    })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', 'Effect configured successfully');
                    document.getElementById('effectGroup').value = '';
                    loadEffectConfigs();
                } else {
                    updateStatus('error', result.error || 'Failed to configure effect');
                }
                
            } catch (error) {
                console.error('Error configuring effect:', error);
                updateStatus('error', 'Failed to configure effect: ' + error.message);
            }
        }
        
        async function loadEffectConfigs() {
            try {
                const response = await fetch('/api/effects/config');
                if (!response.ok) return;
                
                const data = await response.json();
                const container = document.getElementById('effect-configs-list');
                
                if (!container) return;
                
                if (!data.configs || data.configs.length === 0) {
                    container.innerHTML = '<p>No effect configurations</p>';
                    return;
                }
                
                container.innerHTML = data.configs.map(config => 
                    `<div style="padding: 10px; background: #333; margin: 5px 0; border-radius: 3px; border: 1px solid #555;">
                        <strong>${config.name}</strong> → Groups: ${config.groups.join(', ')} | Audio: ${config.audioFile ? 'Track ' + config.audioFile : 'None'}
                        <div style="margin-top: 5px;">
                            <button onclick="removeEffectConfig('${config.name}')" class="btn btn-danger" style="padding: 5px 10px;">Remove</button>
                        </div>
                    </div>`
                ).join('');
            } catch (error) {
                console.error('Error loading effect configs:', error);
            }
        }
        
        async function removeEffectConfig(effectName) {
            try {
                const response = await fetch('/api/effects/config', {
                    method: 'DELETE',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name: effectName })
                });
                
                if (response.ok) {
                    updateStatus('success', 'Effect configuration removed');
                    loadEffectConfigs();
                } else {
                    updateStatus('error', 'Failed to remove effect configuration');
                }
            } catch (error) {
                updateStatus('error', 'Failed to remove effect configuration: ' + error.message);
            }
        }
        
        async function loadAvailableEffects() {
            try {
                const response = await fetch('/api/effects');
                if (!response.ok) return;
                
                const data = await response.json();
                const select = document.getElementById('effectName');
                
                if (!select) return;
                
                select.innerHTML = (data.effects || []).map(effect => 
                    `<option value="${effect.name}">${effect.name}</option>`
                ).join('');
            } catch (error) {
                console.error('Error loading available effects:', error);
            }
        }
        
        async function removeAudioTrack(fileNumber) {
            try {
                const response = await fetch(`/api/audio/tracks?fileNumber=${fileNumber}`, {
                    method: 'DELETE'
                });
                
                if (response.ok) {
                    updateStatus('success', 'Audio track removed');
                    loadAudioTracks();
                } else {
                    updateStatus('error', 'Failed to remove audio track');
                }
            } catch (error) {
                updateStatus('error', 'Failed to remove audio track: ' + error.message);
            }
        }
        
        // Load zones from server
        async function loadZones() {
            try {
                const response = await fetch('/api/zones');
                if (!response.ok) throw new Error('Failed to load zones');
                
                const data = await response.json();
                zones = data.zones || [];
                renderZones();
                renderConfiguredZones();
                updateStatus('success', `Loaded ${zones.length} zones`);
            } catch (error) {
                console.error('Error loading zones:', error);
                updateStatus('error', 'Failed to load zones: ' + error.message);
            }
        }
        
        function renderConfiguredZones() {
            const container = document.getElementById('zones-config-list');
            if (!container) return;
            
            if (zones.length === 0) {
                container.innerHTML = '<p>No zones configured</p>';
                return;
            }
            
            container.innerHTML = zones.map(zone => `
                <div style="padding: 10px; background: #333; margin: 5px 0; border-radius: 3px; border: 1px solid #555;">
                    <strong>${zone.name}</strong> - GPIO ${zone.gpio} (${zone.type})
                    <br><small>Group: ${zone.groupName} | Max Brightness: ${zone.brightness} | ${zone.type === 'WS2812B' ? (zone.ledCount || 1) + ' LEDs' : 'Single LED'}</small>
                    <div style="margin-top: 10px; display: flex; align-items: center; gap: 10px;">
                        <label style="min-width: 80px;">Brightness:</label>
                        <input type="range" min="0" max="255" value="${zone.currentBrightness || zone.brightness}" 
                               oninput="setBrightness(${zone.id}, this.value); this.nextElementSibling.textContent = this.value"
                               style="flex: 1; height: 6px; border-radius: 3px; background: #444;">
                        <span style="min-width: 40px; text-align: right; font-weight: bold;">${zone.currentBrightness || zone.brightness}</span>
                    </div>
                    <div style="margin-top: 5px;">
                        <button onclick="removeZone(${zone.id})" class="btn btn-danger" style="padding: 5px 10px;">Remove</button>
                    </div>
                </div>
            `).join('');
        }
        
        async function removeZone(zoneId) {
            try {
                const response = await fetch('/api/zones/' + zoneId, {
                    method: 'DELETE'
                });
                
                if (response.ok) {
                    updateStatus('success', 'Zone removed');
                    loadZones();
                    loadEffects(); // Effects may have changed
                } else {
                    updateStatus('error', 'Failed to remove zone');
                }
            } catch (error) {
                updateStatus('error', 'Failed to remove zone: ' + error.message);
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
        
        // Render zones in UI - simplified for main page (no individual brightness controls)
        function renderZones() {
            // Main page no longer shows individual zone controls
            // Zone control is moved to configuration section
        }
        
        // Global control functions
        function setGlobalBrightness(brightness) {
            document.getElementById('global-brightness-value').textContent = brightness;
            
            // Set brightness for all zones
            zones.forEach(zone => {
                setBrightness(zone.id, brightness);
            });
        }
        
        function setAllZonesBrightness(brightness) {
            zones.forEach(zone => {
                setBrightness(zone.id, brightness);
            });
            
            // Update global slider
            document.getElementById('global-brightness').value = brightness;
            document.getElementById('global-brightness-value').textContent = brightness;
        }
        
        async function stopAllEffects() {
            try {
                updateStatus('loading', 'Stopping all effects...');
                
                const response = await fetch('/api/effects/stop-all', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', 'All effects stopped');
                    setTimeout(() => loadEffects(), 500);
                } else {
                    updateStatus('error', result.error || 'Failed to stop effects');
                }
                
            } catch (error) {
                console.error('Error stopping effects:', error);
                updateStatus('error', 'Failed to stop effects: ' + error.message);
            }
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
            const fileInput = document.getElementById('firmwareFile');
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
        function setupNewZoneForm() {
            // Setup zone type change handler
            const newZoneTypeSelect = document.getElementById('newZoneType');
            if (newZoneTypeSelect) {
                newZoneTypeSelect.addEventListener('change', function() {
                    const ledCountRow = document.getElementById('newLedCountRow');
                    if (ledCountRow) {
                        ledCountRow.style.display = this.value === 'WS2812B' ? 'flex' : 'none';
                    }
                });
            }
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
        
        async function setVolume(volume) {
            // Update display immediately
            document.getElementById('volume-value').textContent = volume;
            
            try {
                const response = await fetch('/api/audio/volume', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ volume: parseInt(volume) })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    // Success - no need to show status for every slider movement
                    console.log('Volume set to', volume);
                } else {
                    console.error('Failed to set volume:', result.error);
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
                
                const statusEl = document.getElementById('audio-status');
                const trackEl = document.getElementById('current-track');
                const availableEl = document.getElementById('audio-available');
                const volumeSlider = document.getElementById('audio-volume');
                const volumeValue = document.getElementById('volume-value');
                
                if (statusEl) statusEl.textContent = data.status || 'Unknown';
                if (trackEl) trackEl.textContent = data.currentTrack > 0 ? data.currentTrack : 'None';
                if (availableEl) availableEl.textContent = data.available ? 'Yes' : 'No';
                
                // Update volume slider
                if (volumeSlider) volumeSlider.value = data.volume || 15;
                if (volumeValue) volumeValue.textContent = data.volume || 15;
                
            } catch (error) {
                console.error('Error getting audio status:', error);
                const statusEl = document.getElementById('audio-status');
                const availableEl = document.getElementById('audio-available');
                if (statusEl) statusEl.textContent = 'Error';
                if (availableEl) availableEl.textContent = 'Error';
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
            
            // Load initial WiFi status
            refreshWiFiStatus();
            
            // Setup show/hide password toggle
            const showPasswordCheckbox = document.getElementById('showPassword');
            const passwordInput = document.getElementById('wifiPassword');
            if (showPasswordCheckbox && passwordInput) {
                showPasswordCheckbox.addEventListener('change', function() {
                    passwordInput.type = this.checked ? 'text' : 'password';
                });
            }
        });
        
        // WiFi configuration functions
        async function saveWiFiConfig() {
            const deviceName = document.getElementById('deviceName').value.trim();
            const ssid = document.getElementById('wifiNetwork').value.trim();
            const password = document.getElementById('wifiPassword').value;
            
            if (!deviceName) {
                updateStatus('error', 'Please enter a device name');
                return;
            }
            
            if (!ssid) {
                updateStatus('error', 'Please enter a network name (SSID)');
                return;
            }
            
            try {
                updateStatus('loading', 'Saving WiFi configuration and connecting...');
                
                const response = await fetch('/api/wifi/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ deviceName, ssid, password })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    // Clear password field for security
                    document.getElementById('wifiPassword').value = '';
                    // Refresh status after a few seconds to see connection result
                    setTimeout(() => refreshWiFiStatus(), 5000);
                } else {
                    updateStatus('error', result.error || 'Failed to save WiFi configuration');
                }
                
            } catch (error) {
                console.error('Error saving WiFi config:', error);
                updateStatus('error', 'Failed to save WiFi configuration: ' + error.message);
            }
        }
        
        async function clearWiFiConfig() {
            if (!confirm('Clear WiFi configuration? The device will switch to AP mode.')) {
                return;
            }
            
            try {
                updateStatus('loading', 'Clearing WiFi configuration...');
                
                const response = await fetch('/api/wifi/clear', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message);
                    document.getElementById('wifiNetwork').value = '';
                    document.getElementById('wifiPassword').value = '';
                    setTimeout(() => refreshWiFiStatus(), 3000);
                } else {
                    updateStatus('error', result.error || 'Failed to clear WiFi configuration');
                }
                
            } catch (error) {
                console.error('Error clearing WiFi config:', error);
                updateStatus('error', 'Failed to clear WiFi configuration: ' + error.message);
            }
        }
        
        async function refreshWiFiStatus() {
            try {
                const response = await fetch('/api/status');
                if (!response.ok) throw new Error('Failed to get status');
                
                const data = await response.json();
                
                // Update WiFi status display
                const wifiConnected = data.wifiConnected || false;
                const wifiMode = wifiConnected ? 'Station Mode' : 'Access Point Mode';
                const ssid = data.wifiSSID || 'BattleAura-' + data.deviceId;
                const ip = data.ip || 'Unknown';
                const hostname = (data.hostname || 'battleaura') + '.local';
                
                document.getElementById('wifi-status').textContent = wifiMode;
                document.getElementById('wifi-ssid').textContent = ssid;
                document.getElementById('wifi-ip').textContent = ip;
                document.getElementById('device-hostname').textContent = hostname;
                
                // Pre-fill current device name and SSID
                if (data.deviceName) {
                    document.getElementById('deviceName').value = data.deviceName;
                }
                if (wifiConnected && data.wifiSSID && data.wifiSSID !== '') {
                    document.getElementById('wifiNetwork').value = data.wifiSSID;
                }
                
            } catch (error) {
                console.error('Error getting WiFi status:', error);
                document.getElementById('wifi-status').textContent = 'Error';
                document.getElementById('wifi-ssid').textContent = 'Error';
                document.getElementById('wifi-ip').textContent = 'Error';
                document.getElementById('device-hostname').textContent = 'Error';
            }
        }
        
        async function saveDeviceConfig() {
            const deviceName = document.getElementById('deviceName').value.trim();
            const audioEnabled = document.getElementById('audioEnabled').checked;
            
            if (!deviceName) {
                updateStatus('error', 'Please enter a device name');
                return;
            }
            
            try {
                updateStatus('loading', 'Saving device configuration...');
                
                const response = await fetch('/api/device/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ deviceName, audioEnabled })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', result.message || 'Device configuration saved');
                } else {
                    updateStatus('error', result.error || 'Failed to save device configuration');
                }
                
            } catch (error) {
                console.error('Error saving device config:', error);
                updateStatus('error', 'Failed to save device configuration: ' + error.message);
            }
        }
        
        async function restartDevice() {
            if (!confirm('Restart the device? This will disconnect the current session.')) {
                return;
            }
            
            try {
                updateStatus('loading', 'Restarting device...');
                
                const response = await fetch('/api/system/restart', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                if (response.ok) {
                    updateStatus('success', 'Device is restarting...');
                } else {
                    updateStatus('error', 'Failed to restart device');
                }
                
            } catch (error) {
                console.error('Error restarting device:', error);
                updateStatus('error', 'Failed to restart device: ' + error.message);
            }
        }
        
        async function factoryReset() {
            if (!confirm('Factory reset will erase ALL configuration and restart the device. This cannot be undone. Are you sure?')) {
                return;
            }
            
            try {
                updateStatus('loading', 'Performing factory reset...');
                
                const response = await fetch('/api/system/factory-reset', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    updateStatus('success', 'Factory reset complete. Device is restarting...');
                } else {
                    updateStatus('error', result.error || 'Failed to perform factory reset');
                }
                
            } catch (error) {
                console.error('Error performing factory reset:', error);
                updateStatus('error', 'Failed to perform factory reset: ' + error.message);
            }
        }
    </script>
</body>
</html>
)rawliteral";


} // namespace BattleAura