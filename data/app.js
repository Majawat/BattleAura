// BattleAura Web Interface JavaScript Application
class BattleAuraApp {
    constructor() {
        this.config = null;
        this.ws = null;
        this.init();
    }

    async init() {
        this.setupEventListeners();
        this.setupWebSocket();
        await this.loadConfig();
        this.renderAll();
        this.updateStatus('connected');
    }

    setupEventListeners() {
        // Tab switching
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                this.switchTab(e.target.dataset.tab);
            });
        });

        // Auto-refresh every 30 seconds
        setInterval(() => this.loadConfig(), 30000);
    }

    setupWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;
        
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.updateStatus('connected');
        };
        
        this.ws.onclose = () => {
            console.log('WebSocket disconnected');
            this.updateStatus('disconnected');
            // Attempt to reconnect after 5 seconds
            setTimeout(() => this.setupWebSocket(), 5000);
        };
        
        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            this.handleWebSocketMessage(data);
        };
    }

    switchTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');

        // Update tab content
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.remove('active');
        });
        document.getElementById(tabName).classList.add('active');
    }

    updateStatus(status) {
        const statusEl = document.getElementById('status');
        if (status === 'connected') {
            statusEl.className = 'status-indicator connected';
            statusEl.innerHTML = '<i class="fas fa-circle"></i> <span>Connected</span>';
        } else {
            statusEl.className = 'status-indicator disconnected';
            statusEl.innerHTML = '<i class="fas fa-circle"></i> <span>Disconnected</span>';
        }
    }

    showLoading() {
        document.getElementById('loading').classList.add('active');
    }

    hideLoading() {
        document.getElementById('loading').classList.remove('active');
    }

    async apiCall(endpoint, options = {}) {
        try {
            const response = await fetch(`/api${endpoint}`, {
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                },
                ...options
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            return await response.json();
        } catch (error) {
            console.error('API call failed:', error);
            throw error;
        }
    }

    async loadConfig() {
        try {
            this.config = await this.apiCall('/config');
            console.log('Config loaded:', this.config);
        } catch (error) {
            console.error('Failed to load config:', error);
        }
    }

    renderAll() {
        if (!this.config) return;
        
        this.renderSystemInfo();
        this.renderPinConfig();
        this.renderAudioTracks();
        this.renderEffectsConfig();
        this.updateControlValues();
    }

    renderSystemInfo() {
        const systemInfo = document.getElementById('system-info');
        systemInfo.innerHTML = `
            <div class="info-item">
                <div class="info-label">Device Name</div>
                <div class="info-value">${this.config.deviceName}</div>
            </div>
            <div class="info-item">
                <div class="info-label">Firmware Version</div>
                <div class="info-value">${this.config.firmwareVersion || 'Unknown'}</div>
            </div>
            <div class="info-item">
                <div class="info-label">Active Pins</div>
                <div class="info-value">${this.config.activePins}</div>
            </div>
            <div class="info-item">
                <div class="info-label">Audio Tracks</div>
                <div class="info-value">${this.config.activeAudioTracks}</div>
            </div>
            <div class="info-item">
                <div class="info-label">Effects</div>
                <div class="info-value">${this.config.activeEffects}</div>
            </div>
            <div class="info-item">
                <div class="info-label">Uptime</div>
                <div class="info-value" id="uptime">--</div>
            </div>
        `;
    }

    renderPinConfig() {
        const pinConfig = document.getElementById('pin-config');
        const effectTypes = ['off', 'candle', 'pulse', 'console', 'static', 'rgb_red', 'rgb_green', 'rgb_blue', 'rgb_pulse_red', 'rgb_pulse_blue'];
        const pinTypes = ['unused', 'led', 'rgb'];

        pinConfig.innerHTML = this.config.pins.map((pin, index) => `
            <div class="pin-item">
                <h4>Pin D${index} ${pin.pin ? `(GPIO ${pin.pin})` : ''}</h4>
                <div class="form-group">
                    <label>Type:</label>
                    <select id="pin-type-${index}">
                        ${pinTypes.map(type => 
                            `<option value="${type}" ${pin.type === type ? 'selected' : ''}>${type}</option>`
                        ).join('')}
                    </select>
                </div>
                <div class="form-group">
                    <label>Label:</label>
                    <input type="text" id="pin-label-${index}" value="${pin.label || ''}" placeholder="Pin label">
                </div>
                <div class="form-group">
                    <label>Effect:</label>
                    <select id="pin-effect-${index}">
                        ${effectTypes.map(effect => 
                            `<option value="${effect}" ${pin.effectType === effect ? 'selected' : ''}>${effect}</option>`
                        ).join('')}
                    </select>
                </div>
                <div class="form-group">
                    <label>Brightness:</label>
                    <input type="range" id="pin-brightness-${index}" min="0" max="255" value="${pin.brightness || 255}">
                    <span id="pin-brightness-value-${index}">${pin.brightness || 255}</span>
                </div>
                <div class="form-group">
                    <label>
                        <input type="checkbox" id="pin-enabled-${index}" ${pin.enabled ? 'checked' : ''}>
                        Enabled
                    </label>
                </div>
            </div>
        `).join('');

        // Add brightness slider listeners
        this.config.pins.forEach((pin, index) => {
            const slider = document.getElementById(`pin-brightness-${index}`);
            const valueSpan = document.getElementById(`pin-brightness-value-${index}`);
            slider.addEventListener('input', (e) => {
                valueSpan.textContent = e.target.value;
            });
        });
    }

    renderAudioTracks() {
        const audioTracks = document.getElementById('audio-tracks');
        const categories = ['system', 'ambient', 'weapon', 'damage', 'victory', 'other'];

        audioTracks.innerHTML = this.config.audioTracks.map((track, index) => `
            <div class="audio-item">
                <div><strong>Track ${track.id}</strong></div>
                <input type="text" id="audio-filename-${index}" value="${track.filename || ''}" placeholder="0001.mp3">
                <input type="text" id="audio-label-${index}" value="${track.label || ''}" placeholder="Track name">
                <select id="audio-category-${index}">
                    ${categories.map(cat => 
                        `<option value="${cat}" ${track.category === cat ? 'selected' : ''}>${cat}</option>`
                    ).join('')}
                </select>
                <input type="range" id="audio-volume-${index}" min="0" max="30" value="${track.volume || 20}" title="Volume: ${track.volume || 20}">
                <label><input type="checkbox" id="audio-enabled-${index}" ${track.enabled ? 'checked' : ''}> Active</label>
            </div>
        `).join('');

        // Add volume slider listeners
        this.config.audioTracks.forEach((track, index) => {
            const slider = document.getElementById(`audio-volume-${index}`);
            slider.addEventListener('input', (e) => {
                slider.title = `Volume: ${e.target.value}`;
            });
        });
    }

    renderEffectsConfig() {
        const effectsConfig = document.getElementById('effects-config');
        const patterns = ['flash', 'pulse', 'fade', 'strobe', 'custom'];

        effectsConfig.innerHTML = this.config.effects.map((effect, index) => `
            <div class="effect-item">
                <h4>${effect.effectId || `Effect ${index + 1}`}</h4>
                <div class="effect-controls">
                    <div class="form-group">
                        <label>Effect ID:</label>
                        <input type="text" id="effect-id-${index}" value="${effect.effectId || ''}" placeholder="weapon1">
                    </div>
                    <div class="form-group">
                        <label>Label:</label>
                        <input type="text" id="effect-label-${index}" value="${effect.label || ''}" placeholder="Machine Gun">
                    </div>
                    <div class="form-group">
                        <label>Primary Pin:</label>
                        <select id="effect-primary-${index}">
                            <option value="-1">None</option>
                            ${this.config.pins.map((pin, pinIndex) => 
                                `<option value="${pin.pin}" ${effect.primaryPin === pin.pin ? 'selected' : ''}>D${pinIndex} - ${pin.label || 'Unlabeled'}</option>`
                            ).join('')}
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Audio Track:</label>
                        <select id="effect-audio-${index}">
                            <option value="0">None</option>
                            ${this.config.audioTracks.map(track => 
                                `<option value="${track.id}" ${effect.audioTrack === track.id ? 'selected' : ''}>${track.id} - ${track.label || track.filename}</option>`
                            ).join('')}
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Pattern:</label>
                        <select id="effect-pattern-${index}">
                            ${patterns.map(pattern => 
                                `<option value="${pattern}" ${effect.effectPattern === pattern ? 'selected' : ''}>${pattern}</option>`
                            ).join('')}
                        </select>
                    </div>
                    <div class="form-group">
                        <label>
                            <input type="checkbox" id="effect-enabled-${index}" ${effect.enabled ? 'checked' : ''}>
                            Enabled
                        </label>
                    </div>
                </div>
            </div>
        `).join('');
    }

    updateControlValues() {
        if (!this.config) return;

        // Update volume slider
        const volumeSlider = document.getElementById('volume-slider');
        const volumeValue = document.getElementById('volume-value');
        volumeSlider.value = this.config.defaultVolume;
        volumeValue.textContent = this.config.defaultVolume;

        // Update brightness slider
        const brightnessSlider = document.getElementById('brightness-slider');
        const brightnessValue = document.getElementById('brightness-value');
        brightnessSlider.value = this.config.defaultBrightness;
        brightnessValue.textContent = `${this.config.defaultBrightness}%`;

        // Update LED toggle
        const ledIcon = document.getElementById('led-icon');
        const ledText = document.getElementById('led-text');
        if (this.config.hasLEDs) {
            ledIcon.className = 'fas fa-lightbulb';
            ledText.textContent = 'LEDs ON';
        } else {
            ledIcon.className = 'far fa-lightbulb';
            ledText.textContent = 'LEDs OFF';
        }
    }

    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'status':
                this.updateSystemStatus(data.data);
                break;
            case 'effect-started':
                this.showEffectNotification(data.data);
                break;
            case 'config-updated':
                this.loadConfig().then(() => this.renderAll());
                break;
        }
    }

    showEffectNotification(effectData) {
        // Could show a toast notification or update UI to indicate effect is running
        console.log('Effect triggered:', effectData);
    }

    async triggerEffect(effectId) {
        this.showLoading();
        try {
            await this.apiCall(`/trigger/${effectId}`, { method: 'POST' });
        } catch (error) {
            alert('Failed to trigger effect: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async updateVolume(value) {
        document.getElementById('volume-value').textContent = value;
        try {
            await this.apiCall('/config/volume', {
                method: 'POST',
                body: JSON.stringify({ volume: parseInt(value) })
            });
        } catch (error) {
            console.error('Failed to update volume:', error);
        }
    }

    async updateBrightness(value) {
        document.getElementById('brightness-value').textContent = `${value}%`;
        try {
            await this.apiCall('/config/brightness', {
                method: 'POST',
                body: JSON.stringify({ brightness: parseInt(value) })
            });
        } catch (error) {
            console.error('Failed to update brightness:', error);
        }
    }

    async toggleLEDs() {
        const newState = !this.config.hasLEDs;
        this.showLoading();
        try {
            await this.apiCall('/config/leds', {
                method: 'POST',
                body: JSON.stringify({ enabled: newState })
            });
            this.config.hasLEDs = newState;
            this.updateControlValues();
        } catch (error) {
            alert('Failed to toggle LEDs: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async savePinConfig() {
        this.showLoading();
        try {
            const updatedPins = this.config.pins.map((pin, index) => ({
                ...pin,
                type: document.getElementById(`pin-type-${index}`).value,
                label: document.getElementById(`pin-label-${index}`).value,
                effectType: document.getElementById(`pin-effect-${index}`).value,
                brightness: parseInt(document.getElementById(`pin-brightness-${index}`).value),
                enabled: document.getElementById(`pin-enabled-${index}`).checked
            }));

            await this.apiCall('/config/pins', {
                method: 'POST',
                body: JSON.stringify({ pins: updatedPins })
            });

            alert('Pin configuration saved successfully!');
            await this.loadConfig();
            this.renderAll();
        } catch (error) {
            alert('Failed to save pin configuration: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async saveAudioConfig() {
        this.showLoading();
        try {
            const updatedTracks = this.config.audioTracks.map((track, index) => ({
                ...track,
                filename: document.getElementById(`audio-filename-${index}`).value,
                label: document.getElementById(`audio-label-${index}`).value,
                category: document.getElementById(`audio-category-${index}`).value,
                volume: parseInt(document.getElementById(`audio-volume-${index}`).value),
                enabled: document.getElementById(`audio-enabled-${index}`).checked
            }));

            await this.apiCall('/config/audio', {
                method: 'POST',
                body: JSON.stringify({ audioTracks: updatedTracks })
            });

            alert('Audio configuration saved successfully!');
            await this.loadConfig();
            this.renderAll();
        } catch (error) {
            alert('Failed to save audio configuration: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async saveEffectsConfig() {
        this.showLoading();
        try {
            const updatedEffects = this.config.effects.map((effect, index) => ({
                ...effect,
                effectId: document.getElementById(`effect-id-${index}`).value,
                label: document.getElementById(`effect-label-${index}`).value,
                primaryPin: parseInt(document.getElementById(`effect-primary-${index}`).value),
                audioTrack: parseInt(document.getElementById(`effect-audio-${index}`).value),
                effectPattern: document.getElementById(`effect-pattern-${index}`).value,
                enabled: document.getElementById(`effect-enabled-${index}`).checked
            }));

            await this.apiCall('/config/effects', {
                method: 'POST',
                body: JSON.stringify({ effects: updatedEffects })
            });

            alert('Effects configuration saved successfully!');
            await this.loadConfig();
            this.renderAll();
        } catch (error) {
            alert('Failed to save effects configuration: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async checkUpdates() {
        this.showLoading();
        try {
            const result = await this.apiCall('/system/check-updates');
            if (result.updateAvailable) {
                if (confirm(`Update available: ${result.version}\n${result.changelog}\n\nInstall now?`)) {
                    await this.apiCall('/system/update', { method: 'POST' });
                    alert('Update started. Device will restart automatically.');
                }
            } else {
                alert('No updates available. You have the latest version.');
            }
        } catch (error) {
            alert('Failed to check for updates: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }

    async factoryReset() {
        if (!confirm('Are you sure you want to factory reset? This will erase all configuration and restart the device.')) {
            return;
        }

        this.showLoading();
        try {
            await this.apiCall('/system/factory-reset', { method: 'POST' });
            alert('Factory reset initiated. Device will restart shortly.');
        } catch (error) {
            alert('Failed to factory reset: ' + error.message);
        } finally {
            this.hideLoading();
        }
    }
}

// Global functions for HTML onclick handlers
let app;

function triggerEffect(effectId) {
    app.triggerEffect(effectId);
}

function updateVolume(value) {
    app.updateVolume(value);
}

function updateBrightness(value) {
    app.updateBrightness(value);
}

function toggleLEDs() {
    app.toggleLEDs();
}

function savePinConfig() {
    app.savePinConfig();
}

function saveAudioConfig() {
    app.saveAudioConfig();
}

function saveEffectsConfig() {
    app.saveEffectsConfig();
}

function checkUpdates() {
    app.checkUpdates();
}

function factoryReset() {
    app.factoryReset();
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    app = new BattleAuraApp();
});