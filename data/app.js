class BattleAuraController {
    constructor() {
        this.ws = null;
        this.config = null;
        this.reconnectInterval = null;
        this.init();
    }
    
    init() {
        this.connectWebSocket();
        this.loadConfig();
        this.setupEventListeners();
    }
    
    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;
        
        try {
            this.ws = new WebSocket(wsUrl);
            
            this.ws.onopen = () => {
                console.log('WebSocket connected');
                this.updateConnectionStatus(true);
                if (this.reconnectInterval) {
                    clearInterval(this.reconnectInterval);
                    this.reconnectInterval = null;
                }
            };
            
            this.ws.onclose = () => {
                console.log('WebSocket disconnected');
                this.updateConnectionStatus(false);
                this.scheduleReconnect();
            };
            
            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.updateConnectionStatus(false);
            };
            
            this.ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this.handleWebSocketMessage(data);
                } catch (e) {
                    console.error('Failed to parse WebSocket message:', e);
                }
            };
        } catch (error) {
            console.error('Failed to create WebSocket:', error);
            this.updateConnectionStatus(false);
            this.scheduleReconnect();
        }
    }
    
    scheduleReconnect() {
        if (!this.reconnectInterval) {
            this.reconnectInterval = setInterval(() => {
                console.log('Attempting to reconnect...');
                this.connectWebSocket();
            }, 5000);
        }
    }
    
    updateConnectionStatus(connected) {
        const statusEl = document.getElementById('connectionStatus');
        if (connected) {
            statusEl.textContent = 'Connected';
            statusEl.className = 'connection-status connected';
        } else {
            statusEl.textContent = 'Disconnected';
            statusEl.className = 'connection-status disconnected';
        }
    }
    
    async loadConfig() {
        try {
            const response = await fetch('/config');
            this.config = await response.json();
            this.updateUI();
        } catch (error) {
            console.error('Failed to load config:', error);
        }
    }
    
    updateUI() {
        if (!this.config) return;
        
        // Update status display
        document.getElementById('deviceName').textContent = this.config.deviceName;
        document.getElementById('version').textContent = this.config.version || '1.0.0';
        document.getElementById('wifiStatus').textContent = this.config.wifiEnabled ? 'Connected' : 'AP Mode';
        document.getElementById('audioStatus').textContent = this.config.audioEnabled ? 'Enabled' : 'Disabled';
        document.getElementById('activePins').textContent = this.config.activePins;
        
        // Update volume slider
        document.getElementById('volumeSlider').value = this.config.volume;
        document.getElementById('volumeValue').textContent = this.config.volume;
        
        // Update pin configurations
        this.updatePinConfigs();
    }
    
    updatePinConfigs() {
        const container = document.getElementById('pinConfigs');
        container.innerHTML = '';
        
        const enabledPins = this.config.pins.filter(pin => pin.enabled);
        
        if (enabledPins.length === 0) {
            container.innerHTML = '<p>No pins configured. Use the configuration API to set up pins.</p>';
            return;
        }
        
        enabledPins.forEach(pin => {
            const pinDiv = document.createElement('div');
            pinDiv.className = 'pin-config';
            
            const effectName = this.getEffectName(pin.effect);
            const pinType = this.getPinTypeName(pin.type);
            
            pinDiv.innerHTML = `
                <div class="pin-header">
                    <span class="pin-name">${pin.name}</span>
                    <span class="pin-number">Pin ${pin.pin}</span>
                </div>
                <div style="margin-bottom: 10px;">
                    <small>Type: ${pinType} | Effect: ${effectName}</small>
                </div>
                <div class="effect-buttons">
                    <button class="btn btn-small btn-primary" onclick="app.triggerPinEffect(${pin.pin}, ${pin.effect})">
                        Trigger Effect
                    </button>
                    <button class="btn btn-small btn-secondary" onclick="app.stopPinEffect(${pin.pin})">
                        Stop
                    </button>
                    ${pin.type === 2 ? `
                        <input type="color" class="color-picker" value="#${pin.color.toString(16).padStart(6, '0')}" 
                               onchange="app.setPinColor(${pin.pin}, this.value)">
                    ` : ''}
                </div>
                ${pin.brightness !== undefined ? `
                    <div class="slider-container" style="margin-top: 10px;">
                        <label>Brightness:</label>
                        <input type="range" min="0" max="255" value="${pin.brightness}" 
                               onchange="app.setPinBrightness(${pin.pin}, this.value)">
                        <span>${pin.brightness}</span>
                    </div>
                ` : ''}
            `;
            
            container.appendChild(pinDiv);
        });
    }
    
    getEffectName(effectType) {
        const effects = {
            0: 'None',
            1: 'Candle Flicker',
            2: 'Engine Pulse',
            3: 'Machine Gun',
            4: 'Flamethrower',
            5: 'Rocket Launcher',
            6: 'Taking Damage',
            7: 'Explosion',
            8: 'Console RGB',
            9: 'Static On',
            10: 'Static Off'
        };
        return effects[effectType] || 'Unknown';
    }
    
    getPinTypeName(pinType) {
        const types = {
            0: 'Disabled',
            1: 'Standard LED',
            2: 'WS2812B RGB',
            3: 'Analog Input',
            4: 'Digital Input'
        };
        return types[pinType] || 'Unknown';
    }
    
    setupEventListeners() {
        // Volume slider
        const volumeSlider = document.getElementById('volumeSlider');
        volumeSlider.addEventListener('input', (e) => {
            document.getElementById('volumeValue').textContent = e.target.value;
        });
    }
    
    sendWebSocketMessage(message) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(message));
        } else {
            console.warn('WebSocket not connected, falling back to HTTP');
            // Fallback to HTTP requests if WebSocket is not available
            this.sendHttpRequest(message);
        }
    }
    
    async sendHttpRequest(message) {
        try {
            const command = message.command;
            
            if (command === 'trigger_effect') {
                await fetch('/trigger', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        pin: message.pin,
                        effect: message.effect,
                        duration: message.duration || 0,
                        ...(message.audio && { audio: message.audio })
                    })
                });
            } else if (command === 'play_audio') {
                await fetch('/audio', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        action: 'play',
                        file: message.file,
                        loop: message.loop ? 'true' : 'false'
                    })
                });
            } else if (command === 'stop_audio') {
                await fetch('/audio', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({ action: 'stop' })
                });
            }
        } catch (error) {
            console.error('HTTP request failed:', error);
        }
    }
    
    handleWebSocketMessage(data) {
        console.log('Received WebSocket message:', data);
        // Handle incoming WebSocket messages here
    }
    
    triggerPinEffect(pin, effect, duration = 0) {
        this.sendWebSocketMessage({
            command: 'trigger_effect',
            pin: pin,
            effect: effect,
            duration: duration
        });
    }
    
    stopPinEffect(pin) {
        this.sendWebSocketMessage({
            command: 'stop_effect',
            pin: pin
        });
    }
    
    setPinBrightness(pin, brightness) {
        // This would need to be implemented on the server side
        console.log(`Setting pin ${pin} brightness to ${brightness}`);
    }
    
    setPinColor(pin, color) {
        // Convert hex color to integer
        const colorInt = parseInt(color.substring(1), 16);
        console.log(`Setting pin ${pin} color to ${colorInt}`);
        // This would need to be implemented on the server side
    }
}

// Global functions for HTML onclick events
function playAudio(file, loop = false) {
    app.sendWebSocketMessage({
        command: 'play_audio',
        file: file,
        loop: loop
    });
}

function stopAudio() {
    app.sendWebSocketMessage({
        command: 'stop_audio'
    });
}

function setVolume(volume) {
    fetch('/audio', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({
            action: 'volume',
            level: volume
        })
    });
    document.getElementById('volumeValue').textContent = volume;
}

function triggerGlobalEffect(effectType) {
    if (!app.config) return;
    
    const enabledPins = app.config.pins.filter(pin => pin.enabled);
    
    enabledPins.forEach(pin => {
        if (effectType === 'explosion') {
            app.triggerPinEffect(pin.pin, 7, 2000); // Explosion effect for 2 seconds
        } else if (effectType === 'damage') {
            app.triggerPinEffect(pin.pin, 6, 1000); // Taking damage effect for 1 second
        }
    });
    
    // Play corresponding audio
    if (effectType === 'explosion') {
        playAudio(7); // Explosion sound
    } else if (effectType === 'damage') {
        playAudio(5); // Taking hits sound
    }
}

function stopAllEffects() {
    if (!app.config) return;
    
    const enabledPins = app.config.pins.filter(pin => pin.enabled);
    enabledPins.forEach(pin => {
        app.stopPinEffect(pin.pin);
    });
    
    stopAudio();
}

// Initialize the app
const app = new BattleAuraController();