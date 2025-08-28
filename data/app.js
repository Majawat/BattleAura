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
    
    let html = '<h3>Available Effects</h3>';
    
    types.forEach(type => {
        html += `<div class="type-group">`;
        html += `<h4>${type.type} (${type.count} ${type.count === 1 ? 'pin' : 'pins'})</h4>`;
        html += `<div class="type-buttons">`;
        
        // Add common effects for this type
        if (type.type.toLowerCase().includes('engine')) {
            html += `<button onclick="triggerTypeEffect('${type.type}', 'idle')">Engine Idle</button>`;
            html += `<button onclick="triggerTypeEffect('${type.type}', 'rev', 2000)">Engine Rev</button>`;
        } else if (type.type.toLowerCase().includes('weapon')) {
            html += `<button onclick="triggerTypeEffect('${type.type}', 'fire', 500)">Weapon Fire</button>`;
            html += `<button onclick="triggerTypeEffect('${type.type}', 'reload', 1000)">Reload</button>`;
        } else if (type.type.toLowerCase().includes('candle')) {
            html += `<button onclick="triggerTypeEffect('${type.type}', 'flicker')">Candle Flicker</button>`;
        } else if (type.type.toLowerCase().includes('console')) {
            html += `<button onclick="triggerTypeEffect('${type.type}', 'scroll')">Data Scroll</button>`;
        }
        
        // Add universal damage effect for all types
        html += `<button onclick="triggerTypeEffect('${type.type}', 'damage', 1500)" class="damage-btn">Taking Damage</button>`;
        
        html += `</div></div>`;
    });
    
    effectsContainer.innerHTML = html;
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

function stopAll() {
    for (let i = 0; i < 8; i++) {
        fetch(`/effects/stop?pin=${i}`);
    }
    console.log('All effects stopped');
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