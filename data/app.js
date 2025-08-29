// BattleAura Web Interface JavaScript

// Initialize page when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    loadSystemInfo();
    loadAvailableTypes();
    setupBrightnessSlider();
    setInterval(updateSystemInfo, 5000); // Update every 5 seconds
});

// Load initial system information
function loadSystemInfo() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('version').textContent = data.version || 'Unknown';
            document.getElementById('memory').textContent = data.freeHeap || 'Unknown';
            document.getElementById('uptime').textContent = data.uptime || 'Unknown';
            updateGpioStatus(data.pins);
        })
        .catch(error => {
            console.log('Status API not available, using basic mode');
            // Fallback for basic functionality
        });
}

// Load available types for dynamic UI generation
function loadAvailableTypes() {
    fetch('/api/types')
        .then(response => response.json())
        .then(data => {
            generateDynamicButtons(data.types);
        })
        .catch(error => {
            console.log('Types API not available, using static interface');
        });
}

// Generate dynamic buttons based on configured types
function generateDynamicButtons(types) {
    const effectsContainer = document.getElementById('dynamic-effects');
    if (!effectsContainer) return;
    
    if (types.length === 0) {
        effectsContainer.innerHTML = `
            <div class="text-center">
                <p class="text-secondary" style="margin: var(--space-6) 0;">No pin types configured yet.</p>
                <a href="/config" class="btn btn-primary">Configure Your Pins →</a>
            </div>
        `;
        return;
    }
    
    let html = '';
    
    // Generate grouped pin-type cards
    types.forEach(type => {
        const effects = getEffectsForType(type.type, type.hasRGB, type.hasPWM);
        
        // Group effects by type
        const ambientEffects = effects.filter(e => e.type === 'ambient');
        const activeEffects = effects.filter(e => e.type === 'active');
        const controlEffects = effects.filter(e => e.type === 'control');
        
        const typeEmoji = getTypeEmoji(type.type);
        const pinCount = `${type.count} ${type.count === 1 ? 'pin' : 'pins'}`;
        
        // Start pin type group card
        html += `
            <div class="pin-type-group">
                <div class="pin-type-header">
                    <span style="font-size: 1.5rem;">${typeEmoji}</span>
                    <h3 class="pin-type-title">${type.type}</h3>
                    <span class="pin-type-count">${pinCount}</span>
                </div>
                <div class="pin-type-sub-cards">
        `;
        
        // Add ambient sub-card
        if (ambientEffects.length > 0) {
            html += `
                <div class="pin-type-sub-card ambient">
                    <div class="sub-card-header">
                        <div class="sub-card-title ambient">🔄 Ambient</div>
                        <div class="sub-card-description">Continuous effects</div>
                    </div>
                    <div class="sub-card-buttons">
            `;
            ambientEffects.forEach(effect => {
                html += `<button onclick="triggerTypeEffect('${type.type}', '${effect.action}', 0, 'ambient')" class="btn btn-success" title="${effect.description}">${effect.label}</button>`;
            });
            html += `
                    </div>
                </div>
            `;
        }
        
        // Add active sub-card
        if (activeEffects.length > 0) {
            html += `
                <div class="pin-type-sub-card active">
                    <div class="sub-card-header">
                        <div class="sub-card-title active">⚡ Active</div>
                        <div class="sub-card-description">Temporary triggers</div>
                    </div>
                    <div class="sub-card-buttons">
            `;
            activeEffects.forEach(effect => {
                html += `<button onclick="triggerTypeEffect('${type.type}', '${effect.action}', 0, 'active')" class="btn btn-warning" title="${effect.description}">${effect.label}</button>`;
            });
            html += `
                    </div>
                </div>
            `;
        }
        
        // Add control sub-card
        if (controlEffects.length > 0) {
            html += `
                <div class="pin-type-sub-card control">
                    <div class="sub-card-header">
                        <div class="sub-card-title control">🎛️ Manual</div>
                        <div class="sub-card-description">Direct control</div>
                    </div>
                    <div class="sub-card-buttons">
            `;
            controlEffects.forEach(effect => {
                html += `<button onclick="triggerTypeEffect('${type.type}', '${effect.action}', 0, 'control')" class="btn btn-info" title="${effect.description}">${effect.label}</button>`;
            });
            html += `
                    </div>
                </div>
            `;
        }
        
        // Close pin type group card
        html += `
                </div>
            </div>
        `;
    });
    
    // Universal Effects as separate group
    html += `
        <div class="pin-type-group">
            <div class="pin-type-header">
                <span style="font-size: 1.5rem;">🎯</span>
                <h3 class="pin-type-title">Global Effects</h3>
                <span class="pin-type-count">All Pins</span>
            </div>
            <div class="pin-type-sub-cards">
                <div class="pin-type-sub-card control">
                    <div class="sub-card-header">
                        <div class="sub-card-title control">💥 Combat</div>
                        <div class="sub-card-description">Damage & explosions</div>
                    </div>
                    <div class="sub-card-buttons">
                        <button onclick="triggerGlobalEffect('damage')" class="btn btn-primary">💥 Taking Damage</button>
                        <button onclick="triggerGlobalEffect('explosion')" class="btn btn-primary">🔥 Explosion</button>
                    </div>
                </div>
                <div class="pin-type-sub-card control">
                    <div class="sub-card-header">
                        <div class="sub-card-title control">⏹️ System</div>
                        <div class="sub-card-description">System controls</div>
                    </div>
                    <div class="sub-card-buttons">
                        <button onclick="stopAllEffects()" class="btn btn-secondary">⏹️ Stop All Effects</button>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    effectsContainer.innerHTML = html;
}

// Helper function to get appropriate CSS class for type
function getTypeClass(typeName) {
    const name = typeName.toLowerCase();
    if (name.includes('engine')) return 'btn-engine';
    if (name.includes('machinegun')) return 'btn-weapon';
    if (name.includes('flamethrower')) return 'btn-weapon';
    if (name.includes('rocketlauncher')) return 'btn-weapon';
    if (name.includes('maincannon')) return 'btn-weapon';
    if (name.includes('weapon')) return 'btn-weapon';
    if (name.includes('candle')) return 'btn-candle';
    if (name.includes('console')) return 'btn-console';
    return 'btn-ambient';
}

// Helper function to get emoji for type
function getTypeEmoji(typeName) {
    const name = typeName.toLowerCase();
    if (name.includes('engine')) return '🚗';
    if (name.includes('machinegun')) return '🔫';
    if (name.includes('flamethrower')) return '🔥';
    if (name.includes('rocketlauncher')) return '🚀';
    if (name.includes('maincannon')) return '💥';
    if (name.includes('weapon')) return '⚔️';
    if (name.includes('candle')) return '🕯️';
    if (name.includes('console')) return '💻';
    if (name.includes('damage')) return '💥';
    return '⚡';
}

// Helper function to get available effects for a type
function getEffectsForType(typeName, hasRGB, hasPWM) {
    const name = typeName.toLowerCase();
    const effects = [];
    
    if (name.includes('engine')) {
        effects.push({ action: 'idle', label: '🔄 Engine Idle', type: 'ambient', description: 'Continuous engine running' });
        effects.push({ action: 'rev', label: '⚡ Engine Rev', type: 'active', description: 'Temporary engine acceleration' });
    } else if (name.includes('machinegun')) {
        effects.push({ action: 'fire', label: '🔫 Machine Gun Burst', type: 'active', description: 'Burst fire effect' });
    } else if (name.includes('flamethrower')) {
        effects.push({ action: 'fire', label: '🔥 Flamethrower', type: 'active', description: 'Flame whoosh effect' });
    } else if (name.includes('rocketlauncher')) {
        effects.push({ action: 'fire', label: '🚀 Rocket Launch', type: 'active', description: 'Rocket launch effect' });
    } else if (name.includes('maincannon')) {
        effects.push({ action: 'fire', label: '💥 Cannon Blast', type: 'active', description: 'Heavy cannon fire' });
    } else if (name.includes('weapon')) {
        // Legacy weapon support - shows all weapon effects
        effects.push({ action: 'fire', label: 'Weapon Fire', type: 'active', description: 'Generic weapon fire' });
        effects.push({ action: 'flamethrower', label: 'Flamethrower', type: 'active', description: 'Flame effect' });
        effects.push({ action: 'rocket', label: 'Rocket', type: 'active', description: 'Rocket effect' });
    } else if (name.includes('candle')) {
        effects.push({ action: 'flicker', label: '🕯️ Candle Flicker', type: 'ambient', description: 'Continuous flickering' });
        effects.push({ action: 'on', label: '💡 Bright', type: 'ambient', description: 'Steady bright light' });
        effects.push({ action: 'fade', label: '🌙 Fade', type: 'ambient', description: 'Gentle fading effect' });
    } else if (name.includes('console')) {
        if (hasRGB) {
            effects.push({ action: 'scroll', label: '📊 Data Scroll', type: 'ambient', description: 'Continuous data display' });
            effects.push({ action: 'pulse', label: '⚠️ Alert Pulse', type: 'active', description: 'Alert notification' });
            effects.push({ action: 'fade', label: '🌙 Fade', type: 'ambient', description: 'Gentle console glow' });
        } else {
            effects.push({ action: 'pulse', label: '💓 Status Pulse', type: 'ambient', description: 'System status indicator' });
        }
    } else if (name.includes('damage')) {
        effects.push({ action: 'damage', label: '💥 Damage Sparks', type: 'active', description: 'Hit impact effect' });
        effects.push({ action: 'strobe', label: '⚡ System Failure', type: 'active', description: 'Critical system alert' });
    } else if (name.includes('ambient')) {
        effects.push({ action: 'pulse', label: '💓 Pulse', type: 'ambient', description: 'Rhythmic pulsing' });
        effects.push({ action: 'strobe', label: '⚡ Strobe', type: 'active', description: 'Fast strobe effect' });
        effects.push({ action: 'fade', label: '🌙 Fade', type: 'ambient', description: 'Smooth fading' });
    } else {
        // Generic type - add basic effects
        effects.push({ action: 'pulse', label: '💓 Pulse', type: 'ambient', description: 'Rhythmic pulsing' });
        effects.push({ action: 'strobe', label: '⚡ Strobe', type: 'active', description: 'Fast strobe effect' });
        effects.push({ action: 'fade', label: '🌙 Fade', type: 'ambient', description: 'Smooth fading' });
    }
    
    // Add universal controls for all types  
    effects.push({ action: 'on', label: '💡 On', type: 'control', description: 'Turn on steady light' });
    effects.push({ action: 'off', label: '⚫ Off', type: 'control', description: 'Turn off completely' });
    effects.push({ action: 'damage', label: '💥 Taking Damage', type: 'active', description: 'Damage/hit response' });
    
    return effects;
}

// Update system information periodically
function updateSystemInfo() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('memory').textContent = data.freeHeap || 'Unknown';
            document.getElementById('uptime').textContent = data.uptime || 'Unknown';
        })
        .catch(error => {
            // Silently handle errors for periodic updates
        });
}

// Update GPIO status display
function updateGpioStatus(pins) {
    const gpioStatus = document.getElementById('gpio-status');
    console.log(pins);
    if (!pins || pins.length === 0) {
        gpioStatus.innerHTML = '<p><em>No output pins configured</em></p>';
        return;
    }
    
    let html = '';
    pins.forEach((pin, index) => {
        if (pin.enabled && (pin.mode === 1 || pin.mode === 2)) { // OUTPUT_DIGITAL or OUTPUT_PWM
            html += `<p><strong>${pin.name} (GPIO ${pin.gpio}):</strong> ${pin.state ? 'ON' : 'OFF'}</p>`;
        }
    });
    
    if (html === '') {
        html = '<p><em>No output pins configured</em></p>';
    }
    
    gpioStatus.innerHTML = html;
}

// Lighting Effects Functions
function startEffect(pin, effect) {
    fetch(`/effects/start?pin=${pin}&effect=${effect}`)
        .then(response => response.text())
        .then(data => console.log(`Effect started: ${data}`))
        .catch(error => console.error('Effect error:', error));
}

function groupEffect(group, effect) {
    fetch(`/effects/group?group=${group}&effect=${effect}`)
        .then(response => response.text())
        .then(data => console.log(`Group effect: ${data}`))
        .catch(error => console.error('Group effect error:', error));
}

// RGB Control Functions (using new modular API)
function setRgbColor(color) {
    // Find first RGB pin and set its color
    fetch('/pin/color?color=' + color)
        .then(response => response.text())
        .then(data => {
            console.log(`RGB color set: ${data}`);
            showFeedback(`RGB ${color.toUpperCase()}`, 'success');
        })
        .catch(error => {
            console.error('RGB error:', error);
            showFeedback('RGB Error', 'error');
        });
}

// Type-based effect functions for modular system
function triggerTypeEffect(type, effect, duration = 0, intent = 'auto') {
    const formData = new FormData();
    formData.append('type', type);
    formData.append('effect', effect);
    if (duration > 0) formData.append('duration', duration);
    if (intent !== 'auto') formData.append('intent', intent);
    
    fetch('/effect', {
        method: 'POST',
        body: formData
    })
        .then(response => response.text())
        .then(data => {
            console.log(`Type effect: ${data}`);
            showFeedback(`${type} ${effect}`, 'success');
        })
        .catch(error => {
            console.error('Type effect error:', error);
            showFeedback('Effect Error', 'error');
        });
}

// Global effects that target all pins
function triggerGlobalEffect(effect) {
    // Use the global endpoint for effects that should affect all pins
    const formData = new FormData();
    formData.append('effect', effect);
    
    fetch('/global', {
        method: 'POST',
        body: formData
    })
        .then(response => response.text())
        .then(data => {
            console.log(`Global effect: ${data}`);
            showFeedback(`Global ${effect}`, 'success');
        })
        .catch(error => {
            console.error('Global effect error:', error);
            showFeedback('Global Effect Error', 'error');
        });
}

// Stop all active effects
function stopAllEffects() {
    fetch('/global', {
        method: 'POST',
        body: new URLSearchParams({ effect: 'stop' })
    })
        .then(response => response.text())
        .then(data => {
            console.log(`Stop all: ${data}`);
            showFeedback('All Effects Stopped', 'success');
        })
        .catch(error => {
            console.error('Stop all error:', error);
            showFeedback('Stop Error', 'error');
        });
}

// Brightness Control Functions
function setBrightness(value) {
    fetch(`/brightness?value=${value}`)
        .then(response => response.text())
        .then(data => {
            console.log(`Brightness set: ${data}`);
            document.getElementById('globalBrightness').value = value;
            document.getElementById('brightnessValue').textContent = value;
            showFeedback(`Brightness: ${Math.round(value/255*100)}%`, 'success');
        })
        .catch(error => {
            console.error('Brightness error:', error);
            showFeedback('Brightness Error', 'error');
        });
}

function setupBrightnessSlider() {
    const slider = document.getElementById('globalBrightness');
    const valueDisplay = document.getElementById('brightnessValue');
    
    // Only setup if elements exist (not all pages have brightness slider)
    if (!slider || !valueDisplay) {
        return;
    }
    
    slider.addEventListener('input', function() {
        const value = this.value;
        valueDisplay.textContent = value;
        
        // Debounce the API calls
        clearTimeout(slider.debounceTimeout);
        slider.debounceTimeout = setTimeout(() => {
            setBrightness(value);
        }, 200);
    });
}

// User Feedback Functions
function showFeedback(message, type) {
    // Create feedback element if it doesn't exist
    let feedback = document.getElementById('feedback');
    if (!feedback) {
        feedback = document.createElement('div');
        feedback.id = 'feedback';
        feedback.style.position = 'fixed';
        feedback.style.top = '20px';
        feedback.style.right = '20px';
        feedback.style.padding = '10px 15px';
        feedback.style.borderRadius = '4px';
        feedback.style.zIndex = '1000';
        feedback.style.fontWeight = 'bold';
        document.body.appendChild(feedback);
    }
    
    // Set message and styling
    feedback.textContent = message;
    feedback.className = type === 'error' ? 'feedback-error' : 'feedback-success';
    
    // Style based on type
    if (type === 'error') {
        feedback.style.background = '#f44336';
        feedback.style.color = 'white';
    } else {
        feedback.style.background = '#4CAF50';
        feedback.style.color = 'white';
    }
    
    // Show and auto-hide
    feedback.style.display = 'block';
    feedback.style.opacity = '1';
    
    setTimeout(() => {
        feedback.style.opacity = '0';
        setTimeout(() => {
            feedback.style.display = 'none';
        }, 300);
    }, 2000);
}

// Utility Functions
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// Error Handling
window.addEventListener('error', function(e) {
    console.error('JavaScript error:', e.error);
});

// Service Worker Registration (for future offline support)
if ('serviceWorker' in navigator) {
    window.addEventListener('load', function() {
        // Future: register service worker for offline functionality
    });
}

// Firmware Update Functions
let updateInfo = null;

function checkForUpdates() {
    const checkBtn = document.getElementById('check-updates-btn');
    checkBtn.disabled = true;
    checkBtn.textContent = '🔍 Checking...';
    
    fetch('/api/update/check')
        .then(response => response.json())
        .then(data => {
            if (data.hasUpdate) {
                updateInfo = data;
                showUpdateNotification(data);
            } else {
                showFeedback('No updates available', 'success');
            }
        })
        .catch(error => {
            console.error('Update check error:', error);
            showFeedback('Update check failed', 'error');
        })
        .finally(() => {
            checkBtn.disabled = false;
            checkBtn.textContent = '🔍 Check for Updates';
        });
}

function showUpdateNotification(data) {
    const notification = document.getElementById('update-notification');
    const details = document.getElementById('update-details');
    
    details.innerHTML = `
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: var(--space-3); margin-bottom: var(--space-3);">
            <div class="status-item">
                <div class="status-label">Current Version</div>
                <div class="status-value">${data.currentVersion}</div>
            </div>
            <div class="status-item">
                <div class="status-label">Latest Version</div>
                <div class="status-value text-success">${data.latestVersion}</div>
            </div>
        </div>
        <div style="background: var(--bg-tertiary); padding: var(--space-3); border-radius: var(--radius-sm);">
            <strong>What's New:</strong><br>
            <span style="color: var(--text-secondary);">${data.changelog || 'Bug fixes and improvements'}</span>
        </div>
        <div style="background: rgba(239, 68, 68, 0.1); border: 1px solid #ef4444; padding: var(--space-3); border-radius: var(--radius-sm); margin-top: var(--space-3);">
            <strong style="color: #fca5a5;">⚠️ Important:</strong> Device will restart after update. Ensure device stays powered during installation.
        </div>
    `;
    
    notification.style.display = 'block';
    notification.scrollIntoView({ behavior: 'smooth' });
}

function downloadUpdate() {
    if (!updateInfo || !updateInfo.downloadUrl) {
        showFeedback('Update information not available', 'error');
        return;
    }
    
    // Require explicit user confirmation
    const confirmed = confirm(
        `Are you sure you want to download and install firmware ${updateInfo.latestVersion}?\n\n` +
        `This will:\n` +
        `• Download firmware from battlesync.me\n` +
        `• Install the update automatically\n` +
        `• Restart the device\n\n` +
        `Device must stay powered during installation!\n\n` +
        `Continue with update?`
    );
    
    if (!confirmed) {
        return;
    }
    
    const downloadBtn = document.getElementById('download-btn');
    downloadBtn.disabled = true;
    downloadBtn.innerHTML = '⏳ Downloading...';
    
    const formData = new FormData();
    formData.append('url', updateInfo.downloadUrl);
    formData.append('confirmed', 'true');
    
    fetch('/api/update/download', {
        method: 'POST',
        body: formData
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'starting') {
            downloadBtn.innerHTML = '🔄 Installing...';
            showFeedback('Update installing - device will restart', 'info');
            
            // Show progress message
            setTimeout(() => {
                document.body.innerHTML = `
                    <div style="font-family: var(--font-body); background: var(--bg-primary); color: var(--text-primary); text-align:center; padding:50px; min-height: 100vh; display: flex; flex-direction: column; justify-content: center;">
                        <h1>🔄 Installing Firmware Update</h1>
                        <p>Please wait while the update installs...</p>
                        <p style="color: var(--warning);">⚠️ Keep device powered - do not disconnect!</p>
                        <div style="margin: 20px auto; width: 200px; height: 4px; background: var(--bg-tertiary); border-radius: 2px; overflow: hidden;">
                            <div style="width: 100%; height: 100%; background: linear-gradient(90deg, var(--primary), var(--primary-light)); animation: pulse 2s infinite;"></div>
                        </div>
                        <p style="font-size: 0.9em; color: var(--text-secondary);">Device will restart automatically when complete</p>
                    </div>
                `;
            }, 2000);
        } else {
            downloadBtn.disabled = false;
            downloadBtn.innerHTML = '📥 Download & Install Update';
            showFeedback(data.message || 'Update failed', 'error');
        }
    })
    .catch(error => {
        console.error('Update download error:', error);
        downloadBtn.disabled = false;
        downloadBtn.innerHTML = '📥 Download & Install Update';
        showFeedback('Update download failed', 'error');
    });
}

function dismissUpdate() {
    const notification = document.getElementById('update-notification');
    notification.style.display = 'none';
    updateInfo = null;
}

// Global control functions
function updateGlobalBrightness(value) {
    document.getElementById('brightnessValue').textContent = value;
    
    fetch('/brightness', {
        method: 'GET',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    })
    .then(() => {
        fetch('/brightness?value=' + value, { method: 'GET' })
        .then(response => response.text())
        .then(data => {
            console.log('Brightness updated:', data);
            showFeedback(`Brightness: ${Math.round(value/255*100)}%`, 'success');
        });
    })
    .catch(error => {
        console.error('Brightness update error:', error);
        showFeedback('Brightness update failed', 'error');
    });
}

function setGlobalBrightness(value) {
    document.getElementById('globalBrightness').value = value;
    updateGlobalBrightness(value);
}

function updateAudioVolume(value) {
    document.getElementById('volumeValue').textContent = value;
    
    fetch('/audio/volume?volume=' + value, { method: 'GET' })
    .then(response => response.text())
    .then(data => {
        console.log('Volume updated:', data);
        const percentage = Math.round(value/30*100);
        showFeedback(`Volume: ${percentage}%`, 'success');
    })
    .catch(error => {
        console.error('Volume update error:', error);
        showFeedback('Volume update failed', 'error');
    });
}

function setAudioVolume(value) {
    document.getElementById('audioVolume').value = value;
    updateAudioVolume(value);
}

// Load current values on page load
async function loadGlobalSettings() {
    try {
        // Load current brightness
        const configResponse = await fetch('/api/config');
        const config = await configResponse.json();
        
        if (config.globalMaxBrightness !== undefined) {
            document.getElementById('globalBrightness').value = config.globalMaxBrightness;
            document.getElementById('brightnessValue').textContent = config.globalMaxBrightness;
        }
        
        // Load current volume
        const audioResponse = await fetch('/audio/status');
        const audioStatus = await audioResponse.json();
        
        if (audioStatus.volume !== undefined) {
            document.getElementById('audioVolume').value = audioStatus.volume;
            document.getElementById('volumeValue').textContent = audioStatus.volume;
        }
    } catch (error) {
        console.error('Failed to load global settings:', error);
    }
}