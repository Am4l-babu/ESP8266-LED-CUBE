/*
 * music_controller.h - Music Reactive Controller for LED Cube
 *
 * Receives real-time music data from the web browser via WebSocket:
 *   - Spotify audio features (tempo, energy, danceability, valence)
 *   - Microphone FFT spectral bands
 *   - Beat detection triggers
 *
 * Maps music features to animation parameters and selects
 * appropriate animations in AI mode.
 */

#ifndef MUSIC_CONTROLLER_H
#define MUSIC_CONTROLLER_H

#include <Arduino.h>

// --- Music Visualization Modes ---
enum MusicMode {
    MODE_SPOTIFY = 0,
    MODE_MICROPHONE,
    MODE_HYBRID,
    MODE_AI,
    MODE_COUNT
};

static const char* const MODE_NAMES[] = {
    "Spotify Sync", "Microphone", "Hybrid", "AI Auto"
};

// --- Music Data (received from browser) ---
struct MusicData {
    // Core
    float tempo;          // BPM (0-300)
    float energy;         // 0.0-1.0
    float danceability;   // 0.0-1.0
    float loudness;       // dB (-60 to 0)
    float valence;        // 0.0-1.0 (happiness)
    float progress;       // track progress in seconds
    bool  beat;           // true on beat onset

    // Spectral (4 bands: sub-bass, bass, mid, high)
    float spectral[4];    // 0.0-1.0 per band

    // Metadata
    bool  active;         // true when receiving data
    unsigned long lastUpdate;
};

// --- AI Pattern Parameters ---
struct AIParams {
    uint8_t intensity;    // 0-255
    uint8_t randomness;   // 0-255
    uint8_t complexity;   // 0-255
};

class MusicController {
public:
    MusicController();

    void begin();
    void update();  // Call in main loop

    // --- Data Input ---
    void setMusicData(float tempo, float energy, float danceability,
                      float loudness, float valence, float progress, bool beat);
    void setSpectralData(float band0, float band1, float band2, float band3);

    // --- Mode ---
    void setMode(MusicMode mode);
    MusicMode getMode() const;
    const char* getModeName() const;

    // --- AI Parameters ---
    void setAIIntensity(uint8_t val);
    void setAIRandomness(uint8_t val);
    void setAIComplexity(uint8_t val);
    AIParams getAIParams() const;

    // --- Getters ---
    const MusicData& getData() const;
    bool isActive() const;
    bool isBeat() const;

    // --- AI Auto-Select ---
    // Returns suggested animation index based on current music features.
    // Call periodically (~every 10s or on track change).
    int suggestAnimation() const;

    // Returns mapped speed for current tempo (higher BPM = lower ms)
    uint16_t getMappedSpeed() const;

    // Returns mapped brightness for current energy
    uint8_t getMappedBrightness() const;

    // Returns mapped density for current danceability
    uint8_t getMappedDensity() const;

private:
    MusicData data;
    MusicMode mode;
    AIParams aiParams;

    unsigned long lastAISwitchTime;
    uint32_t aiSwitchInterval;  // ms between AI pattern changes

    void clearBeat();  // Reset beat flag after processing
};

#endif // MUSIC_CONTROLLER_H
