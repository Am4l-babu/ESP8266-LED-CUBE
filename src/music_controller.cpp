/*
 * music_controller.cpp - Music Reactive Controller Implementation
 *
 * Processes real-time music data from the browser and maps it to
 * LED cube animation parameters. Supports Spotify, microphone FFT,
 * hybrid, and AI auto-select modes.
 */

#include "music_controller.h"

// First music animation index (after the 20 original animations)
#define MUSIC_ANIM_START 20

MusicController::MusicController() {
    mode = MODE_SPOTIFY;
    aiParams = {180, 128, 128};  // mid defaults
    lastAISwitchTime = 0;
    aiSwitchInterval = 15000;  // Switch AI pattern every 15s
    memset(&data, 0, sizeof(MusicData));
}

void MusicController::begin() {
    Serial.println(F("[MusicCtrl] Initialized"));
}

void MusicController::update() {
    // Mark inactive if no data received for 5 seconds
    if (data.active && millis() - data.lastUpdate > 5000) {
        data.active = false;
        Serial.println(F("[MusicCtrl] No music data - inactive"));
    }

    // Auto-clear beat after 50ms
    if (data.beat && millis() - data.lastUpdate > 50) {
        data.beat = false;
    }
}

// --- Data Input ---

void MusicController::setMusicData(float tempo, float energy, float danceability,
                                    float loudness, float valence, float progress, bool beat) {
    data.tempo = constrain(tempo, 0, 300);
    data.energy = constrain(energy, 0.0f, 1.0f);
    data.danceability = constrain(danceability, 0.0f, 1.0f);
    data.loudness = constrain(loudness, -60.0f, 0.0f);
    data.valence = constrain(valence, 0.0f, 1.0f);
    data.progress = progress;
    data.beat = beat;
    data.active = true;
    data.lastUpdate = millis();
}

void MusicController::setSpectralData(float b0, float b1, float b2, float b3) {
    data.spectral[0] = constrain(b0, 0.0f, 1.0f);
    data.spectral[1] = constrain(b1, 0.0f, 1.0f);
    data.spectral[2] = constrain(b2, 0.0f, 1.0f);
    data.spectral[3] = constrain(b3, 0.0f, 1.0f);
    data.lastUpdate = millis();
}

// --- Mode ---

void MusicController::setMode(MusicMode m) {
    mode = m;
    Serial.printf("[MusicCtrl] Mode: %s\n", MODE_NAMES[m]);
}

MusicMode MusicController::getMode() const { return mode; }
const char* MusicController::getModeName() const { return MODE_NAMES[mode]; }

// --- AI Parameters ---

void MusicController::setAIIntensity(uint8_t val) { aiParams.intensity = val; }
void MusicController::setAIRandomness(uint8_t val) { aiParams.randomness = val; }
void MusicController::setAIComplexity(uint8_t val) { aiParams.complexity = val; }
AIParams MusicController::getAIParams() const { return aiParams; }

// --- Getters ---

const MusicData& MusicController::getData() const { return data; }
bool MusicController::isActive() const { return data.active; }
bool MusicController::isBeat() const { return data.beat; }

// --- Mapping Functions ---

uint16_t MusicController::getMappedSpeed() const {
    if (!data.active || data.tempo < 10) return 100;
    // BPM 60 → 250ms, BPM 120 → 125ms, BPM 180 → 83ms
    // Formula: speed_ms = 15000 / tempo
    uint16_t spd = (uint16_t)(15000.0f / data.tempo);
    return constrain(spd, 30, 500);
}

uint8_t MusicController::getMappedBrightness() const {
    if (!data.active) return 200;
    // Energy 0→50, Energy 1→255
    return (uint8_t)(50 + data.energy * 205);
}

uint8_t MusicController::getMappedDensity() const {
    if (!data.active) return 4;
    // Danceability 0→1, 1→12
    return (uint8_t)(1 + data.danceability * 11);
}

// --- AI Animation Suggestion ---

int MusicController::suggestAnimation() const {
    if (!data.active) return MUSIC_ANIM_START;  // Default: bass pulse

    float e = data.energy;
    float d = data.danceability;
    float v = data.valence;
    float t = data.tempo;

    // High energy + high danceability → explosive effects
    if (e > 0.7f && d > 0.7f) {
        return MUSIC_ANIM_START + 7;  // Beat Explosion
    }
    // High energy → particles or fire
    if (e > 0.7f) {
        return (t > 140) ? MUSIC_ANIM_START + 4  // Particle Rain (fast)
                         : MUSIC_ANIM_START + 5;  // Music Fire
    }
    // High danceability → rhythmic patterns
    if (d > 0.6f) {
        return (v > 0.5f) ? MUSIC_ANIM_START + 3  // Spiral Beat (happy)
                          : MUSIC_ANIM_START + 6;  // Orbit Sync
    }
    // Mid energy → ripple or freq tower
    if (e > 0.4f) {
        return MUSIC_ANIM_START + 1;  // Freq Tower
    }
    // Low energy → smooth effects
    if (e > 0.2f) {
        return MUSIC_ANIM_START + 2;  // Beat Ripple
    }
    // Very low energy → bass pulse (subtle)
    return MUSIC_ANIM_START + 0;  // Bass Pulse
}

void MusicController::clearBeat() {
    data.beat = false;
}
