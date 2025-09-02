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
    </style>
</head>
<body>
    <div class="container">
        <h1>BattleAura Controller</h1>
        
        <div id="status" class="status loading">
            Loading zones...
        </div>
        
        <div id="zones-container">
            <!-- Zones will be populated here -->
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
            BattleAura v2.0.0 - Phase 1<br>
            <span id="device-info"></span>
        </div>
    </div>

    <script>
        let zones = [];
        
        // Initialize on page load
        document.addEventListener('DOMContentLoaded', function() {
            loadZones();
            loadStatus();
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
    </script>
</body>
</html>
)rawliteral";

} // namespace BattleAura