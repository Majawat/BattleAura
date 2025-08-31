// BattleAura Web Interface - Simple API-based implementation
document.addEventListener('DOMContentLoaded', function() {
    loadSystemStatus();
    loadPinControls();
    loadPinTypes();
    loadGlobalSettings();
    
    // Update system info periodically
    setInterval(loadSystemStatus, 5000);
});

// Load system status from API
async function loadSystemStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        
        // Update system status display
        document.getElementById('version').textContent = data.version || 'Unknown';
        document.getElementById('memory').textContent = formatMemory(data.freeHeap) || 'Unknown';
        document.getElementById('uptime').textContent = formatUptime(data.uptime) || 'Unknown';
        document.getElementById('status').textContent = data.status || 'UNKNOWN';
        
    } catch (error) {
        console.error('Failed to load system status:', error);
    }
}

// Load individual pin controls
async function loadPinControls() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        
        generatePinControls(data.pins || []);
        
    } catch (error) {
        console.error('Failed to load pin controls:', error);
        document.getElementById('pin-controls').innerHTML = 
            '<p class="text-secondary">Unable to load pin controls</p>';
    }
}

// Load pin types and effects then generate dynamic UI
async function loadPinTypes() {
    try {
        // Load both pin types and available effects
        const [typesResponse, effectsResponse] = await Promise.all([
            fetch('/api/types'),
            fetch('/api/effects')
        ]);
        
        const typesData = await typesResponse.json();
        const effectsData = await effectsResponse.json();
        
        generateEffectsUI(typesData.types || [], effectsData.effects || []);
        
    } catch (error) {
        console.error('Failed to load pin types or effects:', error);
        document.getElementById('dynamic-effects').innerHTML = 
            '<p class="text-secondary">Configure pins to see available effects</p>';
    }
}

// Load global settings (brightness and volume)
async function loadGlobalSettings() {
    try {
        const response = await fetch('/api/config');
        const config = await response.json();
        
        // Set brightness slider and display
        if (config.globalMaxBrightness !== undefined) {
            document.getElementById('globalBrightness').value = config.globalMaxBrightness;
            document.getElementById('brightnessValue').textContent = config.globalMaxBrightness;
        }
        
        // Set volume slider and display
        if (config.volume !== undefined) {
            document.getElementById('audioVolume').value = config.volume;
            document.getElementById('volumeValue').textContent = config.volume;
        }
        
    } catch (error) {
        console.error('Failed to load global settings:', error);
    }
}

// Generate individual pin controls UI
function generatePinControls(pins) {
    const container = document.getElementById('pin-controls');
    
    const enabledPins = pins.filter(pin => pin.enabled);
    
    if (!enabledPins || enabledPins.length === 0) {
        container.innerHTML = `
            <div class="text-center">
                <p class="text-secondary" style="margin: var(--space-6) 0;">No pins configured yet.</p>
                <a href="/config" class="btn btn-primary">Configure Your Pins →</a>
            </div>
        `;
        return;
    }
    
    let html = '<div class="pin-controls-grid">';
    
    enabledPins.forEach(pin => {
        const typeEmoji = getTypeEmoji(pin.type || 'generic');
        
        html += `
            <div class="pin-control-card" data-pin="${pin.gpio}">
                <div class="pin-control-header">
                    <span class="pin-emoji">${typeEmoji}</span>
                    <div class="pin-info">
                        <div class="pin-name">${pin.name || 'Pin ' + pin.gpio}</div>
                        <div class="pin-details">GPIO ${pin.gpio} • ${pin.type || 'Generic'}</div>
                    </div>
                    <div class="pin-status ${pin.state ? 'on' : 'off'}">${pin.state ? 'ON' : 'OFF'}</div>
                </div>
                
                <div class="pin-effects">
                    ${generatePinEffectButtons(pin)}
                </div>
            </div>
        `;
    });
    
    html += '</div>';
    container.innerHTML = html;
}

// Generate effect buttons for a specific pin
function generatePinEffectButtons(pin) {
    const type = (pin.type || 'generic').toLowerCase();
    let buttons = '';
    
    // Type-specific effects
    if (type.includes('candle')) {
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'flicker\')" class="btn btn-success btn-sm">🕯️ Flicker</button>';
    } else if (type.includes('engine')) {
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'idle\')" class="btn btn-success btn-sm">🔄 Idle</button>';
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'rev\')" class="btn btn-warning btn-sm">⚡ Rev</button>';
    } else if (type.includes('weapon')) {
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'fire\')" class="btn btn-warning btn-sm">🔫 Fire</button>';
    } else if (type.includes('console')) {
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'scroll\')" class="btn btn-info btn-sm">📊 Scroll</button>';
        buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'pulse\')" class="btn btn-info btn-sm">💓 Pulse</button>';
    }
    
    // Universal controls
    buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'on\')" class="btn btn-secondary btn-sm">💡 On</button>';
    buttons += '<button onclick="triggerPinEffect(' + pin.gpio + ', \'off\')" class="btn btn-secondary btn-sm">⚫ Off</button>';
    
    return buttons;
}

// Generate dynamic effects UI based on configured pin types and available effects
function generateEffectsUI(types, availableEffects) {
    const container = document.getElementById('dynamic-effects');
    
    if (!types || types.length === 0) {
        container.innerHTML = `
            <div class="text-center">
                <p class="text-secondary" style="margin: var(--space-6) 0;">No pin types configured yet.</p>
                <a href="/config" class="btn btn-primary">Configure Your Pins →</a>
            </div>
        `;
        return;
    }
    
    let html = '';
    
    // Generate cards for each pin type
    types.forEach(type => {
        const effects = getEffectsForType(type.type, type.hasRGB, type.hasPWM, availableEffects);
        const typeEmoji = getTypeEmoji(type.type);
        
        html += `
            <div class="pin-type-group">
                <div class="pin-type-header">
                    <span style="font-size: 1.5rem;">${typeEmoji}</span>
                    <h3 class="pin-type-title">${type.type}</h3>
                    <span class="pin-type-count">${type.count} ${type.count === 1 ? 'pin' : 'pins'}</span>
                </div>
                <div class="pin-type-sub-cards">
                    <div class="pin-type-sub-card control">
                        <div class="sub-card-header">
                            <div class="sub-card-title control">🎛️ Controls</div>
                            <div class="sub-card-description">Direct pin control</div>
                        </div>
                        <div class="sub-card-buttons">
                            ${effects.map(effect => 
                                `<button onclick="triggerEffect('${type.type}', '${effect.id}')" class="btn btn-info" title="${effect.description}">${effect.icon} ${effect.name}</button>`
                            ).join('')}
                        </div>
                    </div>
                </div>
            </div>
        `;
    });
    
    // Add global effects
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
                        <div class="sub-card-description">Universal effects</div>
                    </div>
                    <div class="sub-card-buttons">
                        <button onclick="triggerGlobalEffect('damage')" class="btn btn-warning">💥 Taking Damage</button>
                        <button onclick="triggerGlobalEffect('explosion')" class="btn btn-warning">🔥 Explosion</button>
                        <button onclick="stopAllEffects()" class="btn btn-secondary">⏹️ Stop All</button>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    container.innerHTML = html;
}

// Get available effects for a pin type using programmatic data
function getEffectsForType(typeName, hasRGB, hasPWM, availableEffects) {
    if (!availableEffects || availableEffects.length === 0) {
        // Fallback to basic controls if no effects data
        return [
            { id: 'on', name: 'On', description: 'Turn on steady', icon: '💡' },
            { id: 'off', name: 'Off', description: 'Turn off', icon: '⚫' }
        ];
    }
    
    const name = typeName.toLowerCase();
    const effects = [];
    
    // Filter effects based on pin type and capabilities
    availableEffects.forEach(effect => {
        const categories = effect.categories || [];
        let shouldInclude = false;
        
        // Check if effect matches pin type
        if (categories.includes('basic')) {
            shouldInclude = true; // Basic controls for all pins
        } else if (name.includes('engine') && categories.includes('engine')) {
            shouldInclude = true;
        } else if ((name.includes('weapon') || name.includes('gun')) && categories.includes('weapon')) {
            shouldInclude = true;
        } else if (name.includes('cannon') && (categories.includes('cannon') || categories.includes('weapon'))) {
            shouldInclude = true;
        } else if (name.includes('candle') && categories.includes('candle')) {
            shouldInclude = true;
        } else if (name.includes('console') && categories.includes('console')) {
            // For console types, check RGB requirement for RGB-only effects
            if (categories.includes('rgb') && !hasRGB) {
                shouldInclude = false;
            } else {
                shouldInclude = true;
            }
        } else if ((name.includes('damage') || name.includes('wound')) && categories.includes('damage')) {
            shouldInclude = true;
        } else if (categories.includes('generic')) {
            shouldInclude = true; // Generic effects for unknown types
        }
        
        if (shouldInclude) {
            effects.push(effect);
        }
    });
    
    // Ensure we always have basic controls
    const hasOn = effects.some(e => e.id === 'on');
    const hasOff = effects.some(e => e.id === 'off');
    
    if (!hasOff) {
        effects.push({ id: 'off', name: 'Off', description: 'Turn off', icon: '⚫' });
    }
    if (!hasOn) {
        effects.push({ id: 'on', name: 'On', description: 'Turn on steady', icon: '💡' });
    }
    
    return effects;
}

// Get emoji for pin type
function getTypeEmoji(typeName) {
    const name = typeName.toLowerCase();
    if (name.includes('engine')) return '🚗';
    if (name.includes('weapon') || name.includes('gun')) return '🔫';
    if (name.includes('cannon')) return '💥';
    if (name.includes('candle')) return '🕯️';
    if (name.includes('console')) return '💻';
    return '⚡';
}

// Trigger effect on specific pin type
async function triggerEffect(type, effect) {
    try {
        const response = await fetch('/effect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `type=${encodeURIComponent(type)}&effect=${encodeURIComponent(effect)}`
        });
        
        if (response.ok) {
            showFeedback(`${type} ${effect}`, 'success');
        } else {
            showFeedback('Effect failed', 'error');
        }
    } catch (error) {
        console.error('Effect error:', error);
        showFeedback('Effect error', 'error');
    }
}

// Trigger global effect
async function triggerGlobalEffect(effect) {
    try {
        const response = await fetch('/global', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `effect=${encodeURIComponent(effect)}`
        });
        
        if (response.ok) {
            showFeedback(`Global ${effect}`, 'success');
        } else {
            showFeedback('Global effect failed', 'error');
        }
    } catch (error) {
        console.error('Global effect error:', error);
        showFeedback('Global effect error', 'error');
    }
}

// Stop all effects
async function stopAllEffects() {
    await triggerGlobalEffect('off');
}

// Pin brightness controls moved to config page

// Trigger effect on specific pin
async function triggerPinEffect(pin, effect) {
    try {
        const response = await fetch('/pin/effect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `pin=${pin}&effect=${encodeURIComponent(effect)}`
        });
        
        if (response.ok) {
            showFeedback(`Pin ${pin}: ${effect}`, 'success');
        } else {
            showFeedback('Pin effect failed', 'error');
        }
    } catch (error) {
        console.error('Pin effect error:', error);
        showFeedback('Pin effect error', 'error');
    }
}

// Set global brightness
async function setBrightness(value) {
    try {
        const response = await fetch(`/brightness?value=${value}`);
        
        if (response.ok) {
            document.getElementById('brightnessValue').textContent = value;
            showFeedback(`Brightness: ${Math.round(value/255*100)}%`, 'success');
        } else {
            showFeedback('Brightness update failed', 'error');
        }
    } catch (error) {
        console.error('Brightness error:', error);
        showFeedback('Brightness error', 'error');
    }
}

// Set audio volume
async function setAudioVolume(value) {
    try {
        const response = await fetch(`/volume?value=${value}`);
        
        if (response.ok) {
            document.getElementById('volumeValue').textContent = value;
            showFeedback(`Volume: ${Math.round(value/30*100)}%`, 'success');
        } else {
            showFeedback('Volume update failed', 'error');
        }
    } catch (error) {
        console.error('Volume error:', error);
        showFeedback('Volume error', 'error');
    }
}

// Update volume display while dragging
function updateAudioVolume(value) {
    document.getElementById('volumeValue').textContent = value;
    
    // Debounce the API call
    clearTimeout(updateAudioVolume.timeout);
    updateAudioVolume.timeout = setTimeout(() => setAudioVolume(value), 300);
}

// Show user feedback
function showFeedback(message, type) {
    let feedback = document.getElementById('feedback');
    if (!feedback) {
        feedback = document.createElement('div');
        feedback.id = 'feedback';
        feedback.style.cssText = `
            position: fixed; top: 20px; right: 20px; padding: 10px 15px;
            border-radius: 6px; z-index: 1000; font-weight: bold;
            transition: opacity 0.3s ease;
        `;
        document.body.appendChild(feedback);
    }
    
    feedback.textContent = message;
    feedback.style.background = type === 'error' ? '#ef4444' : '#22c55e';
    feedback.style.color = 'white';
    feedback.style.display = 'block';
    feedback.style.opacity = '1';
    
    setTimeout(() => {
        feedback.style.opacity = '0';
        setTimeout(() => feedback.style.display = 'none', 300);
    }, 2000);
}

// Utility functions
function formatMemory(bytes) {
    if (!bytes) return 'Unknown';
    return Math.round(bytes / 1024) + ' KB';
}

function formatUptime(seconds) {
    if (!seconds) return 'Unknown';
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    return hours > 0 ? `${hours}h ${minutes}m` : `${minutes}m`;
}