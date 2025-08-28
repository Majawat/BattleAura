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
            <h3>⚡ Tactical Effects</h3>
            <p>No pin types configured yet.</p>
            <p><a href="/config" style="color: #4CAF50;">Configure your pins →</a></p>
        `;
        return;
    }
    
    let html = `
        <h3>⚡ Tactical Effects</h3>
        <p>Effects based on your configured pin types</p>
    `;
    
    types.forEach(type => {
        const typeClass = getTypeClass(type.type);
        html += `<div class="type-group">`;
        html += `<h4>${getTypeEmoji(type.type)} ${type.type} (${type.count} ${type.count === 1 ? 'pin' : 'pins'})</h4>`;
        html += `<div class="type-buttons">`;
        
        // Add type-specific effects with appropriate styling
        const effects = getEffectsForType(type.type, type.hasRGB, type.hasPWM);
        effects.forEach(effect => {
            html += `<button onclick="triggerTypeEffect('${type.type}', '${effect.action}'" class="btn ${typeClass}">${effect.label}</button>`;
        });
        
        html += `</div></div>`;
    });
    
    // Add universal effects section
    html += `
        <div class="type-group">
            <h4>🎯 Universal Effects</h4>
            <div class="type-buttons">
                <button onclick="triggerGlobalEffect('damage')" class="btn btn-damage">All Taking Damage</button>
                <button onclick="triggerGlobalEffect('explosion')" class="btn btn-damage">Explosion</button>
                <button onclick="stopAllEffects()" class="btn btn-red">Stop All Effects</button>
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
        effects.push({ action: 'idle', label: 'Engine Idle' });
        effects.push({ action: 'rev', label: 'Engine Rev' });
    } else if (name.includes('machinegun')) {
        effects.push({ action: 'fire', label: 'Machine Gun Burst' });
    } else if (name.includes('flamethrower')) {
        effects.push({ action: 'fire', label: 'Flamethrower' });
    } else if (name.includes('rocketlauncher')) {
        effects.push({ action: 'fire', label: 'Rocket Launch' });
    } else if (name.includes('maincannon')) {
        effects.push({ action: 'fire', label: 'Cannon Blast' });
    } else if (name.includes('weapon')) {
        // Legacy weapon support - shows all weapon effects
        effects.push({ action: 'fire', label: 'Weapon Fire' });
        effects.push({ action: 'flamethrower', label: 'Flamethrower' });
        effects.push({ action: 'rocket', label: 'Rocket' });
    } else if (name.includes('candle')) {
        effects.push({ action: 'flicker', label: 'Candle Flicker' });
        effects.push({ action: 'on', label: 'Bright' });
        effects.push({ action: 'fade', label: 'Fade' });
    } else if (name.includes('console')) {
        if (hasRGB) {
            effects.push({ action: 'scroll', label: 'Data Scroll' });
            effects.push({ action: 'pulse', label: 'Alert Pulse' });
            effects.push({ action: 'fade', label: 'Fade' });
        } else {
            effects.push({ action: 'pulse', label: 'Status Pulse' });
        }
    } else if (name.includes('damage')) {
        effects.push({ action: 'damage', label: 'Damage Sparks' });
        effects.push({ action: 'strobe', label: 'System Failure' });
    } else if (name.includes('ambient')) {
        effects.push({ action: 'pulse', label: 'Pulse' });
        effects.push({ action: 'strobe', label: 'Strobe' });
        effects.push({ action: 'fade', label: 'Fade' });
    } else {
        // Generic type - add basic effects
        effects.push({ action: 'pulse', label: 'Pulse' });
        effects.push({ action: 'strobe', label: 'Strobe' });
        effects.push({ action: 'fade', label: 'Fade' });
    }
    
    // Add universal controls for all types
    effects.push({ action: 'on', label: '💡 On' });
    effects.push({ action: 'off', label: '⚫ Off' });
    effects.push({ action: 'damage', label: '💥 Taking Damage' });
    
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
function triggerTypeEffect(type, effect, duration = 0) {
    const formData = new FormData();
    formData.append('type', type);
    formData.append('effect', effect);
    if (duration > 0) formData.append('duration', duration);
    
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