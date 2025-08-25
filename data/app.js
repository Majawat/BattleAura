/**
 * BattleAura Control Interface
 * Frontend JavaScript for ESP32 lighting and audio control
 */

class BattleAuraController {
    constructor() {
        this.config = null;
        this.wsConnected = false;
        this.init();
    }
    
    async init() {
        try {
            await this.loadConfig();
            this.setupEventListeners();
            this.updateUI();
            this.startStatusUpdates();
        } catch (error) {
            console.error('Initialization failed:', error);
            this.showToast('Failed to initialize interface', 'error');
        }
    }
    
    async loadConfig() {
        try {
            const response = await fetch('/api/config');
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            this.config = await response.json();
            console.log('Configuration loaded:', this.config);
        } catch (error) {
            console.error('Failed to load configuration:', error);
            throw error;
        }
    }
    
    setupEventListeners() {
        // Volume slider
        const volumeSlider = document.getElementById('volumeSlider');
        const volumeValue = document.getElementById('volumeValue');
        
        if (volumeSlider && volumeValue) {
            volumeSlider.addEventListener('input', (e) => {
                volumeValue.textContent = e.target.value;
            });
            
            volumeSlider.addEventListener('change', (e) => {
                this.setVolume(e.target.value);
            });
        }
    }
    
    updateUI() {
        if (!this.config) return;
        
        // Update status cards
        this.updateElement('deviceName', this.config.deviceName);
        this.updateElement('version', this.config.version || '1.1.0');
        this.updateElement('wifiStatus', this.config.wifiEnabled ? 'Connected' : 'AP Mode');
        this.updateElement('audioStatus', this.config.audioEnabled ? 'Enabled' : 'Disabled');
        
        // Update volume slider
        const volumeSlider = document.getElementById('volumeSlider');
        const volumeValue = document.getElementById('volumeValue');
        if (volumeSlider && volumeValue) {
            volumeSlider.value = this.config.volume || 15;
            volumeValue.textContent = this.config.volume || 15;
        }
        
        // Update pin controls
        this.updatePinControls();
    }
    
    updateElement(id, value) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = value;
        }
    }
    
    updatePinControls() {
        const container = document.getElementById('pinControls');
        if (!container) return;
        
        container.innerHTML = '';
        
        if (!this.config.pins) {
            container.innerHTML = '<p>No pin configuration available</p>';
            return;
        }
        
        const enabledPins = this.config.pins.filter(pin => pin && pin.enabled);
        
        if (enabledPins.length === 0) {
            container.innerHTML = '<p>No pins configured. Use the <a href="/config.html">configuration page</a> to set up pins.</p>';
            return;
        }
        
        enabledPins.forEach(pin => {
            const pinElement = this.createPinControl(pin);
            container.appendChild(pinElement);
        });
    }
    
    createPinControl(pin) {
        const pinDiv = document.createElement('div');
        pinDiv.className = 'pin-control';
        
        const effectNames = {
            0: 'None', 1: 'Candle Flicker', 2: 'Engine Pulse', 3: 'Machine Gun',
            4: 'Flamethrower', 5: 'Rocket Launcher', 6: 'Taking Damage',
            7: 'Explosion', 8: 'Console RGB', 9: 'Static On', 10: 'Static Off'
        };
        
        const pinModeNames = {
            0: 'Disabled', 1: 'Standard', 2: 'PWM', 3: 'WS2812B', 4: 'Digital Input', 5: 'Analog Input'
        };
        
        pinDiv.innerHTML = `
            <div class="pin-header">
                <span class="pin-name">${pin.name}</span>
                <span class="pin-number">GPIO ${pin.pin}</span>
            </div>
            <div class="pin-details">
                Mode: ${pinModeNames[pin.pinMode] || 'Unknown'} | 
                Effect: ${effectNames[pin.defaultEffect] || 'Unknown'} |
                Brightness: ${pin.brightness || 255}
            </div>
            <div class="pin-buttons">
                <button class="btn btn-small btn-primary" onclick="app.triggerPinEffect(${pin.pin}, ${pin.defaultEffect})">
                    Trigger Effect
                </button>
                <button class="btn btn-small btn-secondary" onclick="app.setPinState(${pin.pin}, true)">
                    Turn On
                </button>
                <button class="btn btn-small btn-secondary" onclick="app.setPinState(${pin.pin}, false)">
                    Turn Off
                </button>
            </div>
        `;
        
        return pinDiv;
    }
    
    startStatusUpdates() {
        // Poll for status updates every 30 seconds
        setInterval(async () => {
            try {
                const response = await fetch('/api/status');
                if (response.ok) {
                    const status = await response.json();
                    console.log('Status update:', status);
                }
            } catch (error) {
                console.error('Status update failed:', error);
            }
        }, 30000);
    }
    
    // API Methods
    async triggerPinEffect(pin, effect, duration = 0) {
        try {
            const response = await fetch('/api/pin/effect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin, effect, duration })
            });
            
            if (response.ok) {
                this.showToast(`Effect triggered on GPIO ${pin}`, 'success');
            } else {
                throw new Error(`HTTP ${response.status}`);
            }
        } catch (error) {
            console.error('Failed to trigger effect:', error);
            this.showToast('Failed to trigger effect', 'error');
        }
    }
    
    async setPinState(pin, state) {
        try {
            const response = await fetch('/api/pin/state', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin, state })
            });
            
            if (response.ok) {
                this.showToast(`GPIO ${pin} ${state ? 'ON' : 'OFF'}`, 'success');
            } else {
                throw new Error(`HTTP ${response.status}`);
            }
        } catch (error) {
            console.error('Failed to set pin state:', error);
            this.showToast('Failed to set pin state', 'error');
        }
    }
    
    async setVolume(volume) {
        try {
            const response = await fetch('/api/audio/volume', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ volume: parseInt(volume) })
            });
            
            if (response.ok) {
                this.showToast(`Volume set to ${volume}`, 'success');
            } else {
                throw new Error(`HTTP ${response.status}`);
            }
        } catch (error) {
            console.error('Failed to set volume:', error);
            this.showToast('Failed to set volume', 'error');
        }
    }
    
    showToast(message, type = 'success') {
        const toast = document.getElementById('toast');
        if (!toast) return;
        
        toast.textContent = message;
        toast.className = `toast ${type} show`;
        
        setTimeout(() => {
            toast.classList.remove('show');
        }, 3000);
    }
}

// Global functions for HTML onclick events
async function playAudio(fileNumber, loop = false) {
    try {
        const response = await fetch('/api/audio/play', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ file: fileNumber, loop })
        });
        
        if (response.ok) {
            app.showToast(`Playing audio file ${fileNumber}`, 'success');
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        console.error('Failed to play audio:', error);
        app.showToast('Failed to play audio', 'error');
    }
}

async function stopAudio() {
    try {
        const response = await fetch('/api/audio/stop', { method: 'POST' });
        
        if (response.ok) {
            app.showToast('Audio stopped', 'success');
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        console.error('Failed to stop audio:', error);
        app.showToast('Failed to stop audio', 'error');
    }
}

async function triggerGlobalEffect(effectType) {
    try {
        const response = await fetch('/api/effects/global', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ effect: effectType })
        });
        
        if (response.ok) {
            app.showToast(`Global ${effectType} triggered`, 'success');
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        console.error('Failed to trigger global effect:', error);
        app.showToast('Failed to trigger global effect', 'error');
    }
}

async function stopAllEffects() {
    try {
        const response = await fetch('/api/effects/stop', { method: 'POST' });
        
        if (response.ok) {
            app.showToast('All effects stopped', 'success');
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        console.error('Failed to stop effects:', error);
        app.showToast('Failed to stop effects', 'error');
    }
}

async function reloadConfig() {
    try {
        await app.loadConfig();
        app.updateUI();
        app.showToast('Configuration reloaded', 'success');
    } catch (error) {
        app.showToast('Failed to reload configuration', 'error');
    }
}

async function factoryReset() {
    if (!confirm('Factory reset will erase all settings. Continue?')) {
        return;
    }
    
    try {
        const response = await fetch('/api/system/factory-reset', { method: 'POST' });
        
        if (response.ok) {
            app.showToast('Factory reset initiated', 'warning');
            setTimeout(() => location.reload(), 3000);
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        console.error('Factory reset failed:', error);
        app.showToast('Factory reset failed', 'error');
    }
}

// Initialize the application
const app = new BattleAuraController();