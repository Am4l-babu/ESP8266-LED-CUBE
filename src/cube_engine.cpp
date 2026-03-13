/*
 * cube_engine.cpp - 4x4x4 LED Cube Multiplexing Engine
 * 
 * MULTIPLEXING EXPLAINED:
 * The cube has 4 layers (z=0 to z=3). Each layer has 16 LEDs (4x4 grid).
 * All LEDs in a layer share a common anode, controlled by a PNP transistor.
 * Each column (cathode) is driven by a 74HC595 through a ULN2803.
 * 
 * Only ONE layer is active at any time:
 *   1. Deactivate all layers (PNP OFF = pin HIGH)
 *   2. Shift out column data for the target layer via 74HC595s
 *   3. Latch the data (pulse LATCH_PIN)
 *   4. Activate the target layer (PNP ON = pin LOW)
 *   5. Hold for ~2.5ms
 *   6. Move to next layer
 * 
 * At ~4 layers × 2.5ms = 10ms per full scan = ~100Hz refresh = flicker-free
 * 
 * SHIFT REGISTER CONTROL:
 * Two 74HC595 are daisy-chained (Q7' of first → SER of second).
 * We shift out 16 bits total: first the HIGH byte, then the LOW byte.
 * The ULN2803 INVERTS and SINKS current:
 *   74HC595 HIGH → ULN2803 output LOW → current flows through LED → LED ON
 * 
 * BRIGHTNESS CONTROL:
 * Achieved by varying the layer hold time using the brightness value.
 * Lower brightness = shorter ON time per layer scan.
 */

#include "cube_engine.h"

CubeEngine::CubeEngine() {
    currentLayer = 0;
    brightness = MAX_BRIGHTNESS;
    refreshCount = 0;
    fpsTimer = 0;
    currentFPS = 0;
    lastRefreshMicros = 0;
    
    // Initialize layer pin lookup
    layerPins[0] = LAYER_PIN_0;
    layerPins[1] = LAYER_PIN_1;
    layerPins[2] = LAYER_PIN_2;
    layerPins[3] = LAYER_PIN_3;
    
    // Clear frame buffer
    memset(frameBuffer, 0, sizeof(frameBuffer));
}

void CubeEngine::begin() {
    // Configure shift register pins as outputs
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    
    // Configure layer pins as outputs and deactivate all layers
    // PNP transistors: HIGH = OFF, LOW = ON
    for (uint8_t i = 0; i < CUBE_Z; i++) {
        pinMode(layerPins[i], OUTPUT);
        digitalWrite(layerPins[i], HIGH);  // Layer OFF
    }
    
    // Clear shift registers
    shiftOut16(0x0000);
    
    // Initialize timing
    lastRefreshMicros = micros();
    fpsTimer = millis();
    
    Serial.println(F("[CubeEngine] Initialized - 4x4x4 LED Cube"));
    Serial.printf("[CubeEngine] Pins: DATA=%d, CLOCK=%d, LATCH=%d\n", DATA_PIN, CLOCK_PIN, LATCH_PIN);
    Serial.printf("[CubeEngine] Layers: %d, %d, %d, %d\n", 
                  layerPins[0], layerPins[1], layerPins[2], layerPins[3]);
}

// --- Voxel Operations ---

void CubeEngine::setVoxel(uint8_t x, uint8_t y, uint8_t z, bool state) {
    if (x >= CUBE_X || y >= CUBE_Y || z >= CUBE_Z) return;
    
    uint8_t bitPos = y * CUBE_X + x;  // Column index within layer
    if (state) {
        frameBuffer[z] |= (1 << bitPos);   // Set bit
    } else {
        frameBuffer[z] &= ~(1 << bitPos);  // Clear bit
    }
}

void CubeEngine::clearVoxel(uint8_t x, uint8_t y, uint8_t z) {
    setVoxel(x, y, z, false);
}

bool CubeEngine::getVoxel(uint8_t x, uint8_t y, uint8_t z) {
    if (x >= CUBE_X || y >= CUBE_Y || z >= CUBE_Z) return false;
    uint8_t bitPos = y * CUBE_X + x;
    return (frameBuffer[z] >> bitPos) & 1;
}

void CubeEngine::toggleVoxel(uint8_t x, uint8_t y, uint8_t z) {
    if (x >= CUBE_X || y >= CUBE_Y || z >= CUBE_Z) return;
    uint8_t bitPos = y * CUBE_X + x;
    frameBuffer[z] ^= (1 << bitPos);
}

// --- Layer Operations ---

void CubeEngine::setLayer(uint8_t z, uint16_t pattern) {
    if (z >= CUBE_Z) return;
    frameBuffer[z] = pattern;
}

uint16_t CubeEngine::getLayer(uint8_t z) {
    if (z >= CUBE_Z) return 0;
    return frameBuffer[z];
}

void CubeEngine::fillLayer(uint8_t z, bool state) {
    if (z >= CUBE_Z) return;
    frameBuffer[z] = state ? 0xFFFF : 0x0000;
}

// --- Bulk Operations ---

void CubeEngine::clearAll() {
    memset(frameBuffer, 0, sizeof(frameBuffer));
}

void CubeEngine::fillAll() {
    memset(frameBuffer, 0xFF, sizeof(frameBuffer));
}

void CubeEngine::setBuffer(const uint16_t buf[CUBE_Z]) {
    memcpy(frameBuffer, buf, sizeof(frameBuffer));
}

void CubeEngine::getBuffer(uint16_t buf[CUBE_Z]) {
    memcpy(buf, frameBuffer, sizeof(frameBuffer));
}

// --- Brightness ---

void CubeEngine::setBrightness(uint8_t b) {
    brightness = constrain(b, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
}

uint8_t CubeEngine::getBrightness() {
    return brightness;
}

// --- Refresh (NON-BLOCKING multiplexing scan) ---
// Call this as frequently as possible in loop()
// Each call advances to the next layer

void CubeEngine::refresh() {
    unsigned long now = micros();
    
    // Calculate hold time based on brightness
    // brightness 255 = full hold time, brightness 10 = minimal hold time  
    unsigned long holdTime = map(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS, 
                                 500, LAYER_HOLD_US);
    
    // Check if enough time has passed for current layer
    if (now - lastRefreshMicros < holdTime) return;
    lastRefreshMicros = now;
    
    // STEP 1: Deactivate all layers first (prevents ghosting)
    deactivateAllLayers();
    
    // STEP 2: Shift out column data for current layer
    // The 74HC595 receives 16 bits of column data
    // ULN2803 sinks current: bit=1 → LED cathode pulled LOW → LED ON
    shiftOut16(frameBuffer[currentLayer]);
    
    // STEP 3: Activate only the current layer
    // PNP transistor: pin LOW → transistor ON → layer anode HIGH → LEDs can light
    activateLayer(currentLayer);
    
    // STEP 4: Advance to next layer
    currentLayer = (currentLayer + 1) % CUBE_Z;
    
    // FPS tracking (count full 4-layer scans)
    if (currentLayer == 0) {
        refreshCount++;
        
        // Update FPS every second
        unsigned long nowMs = millis();
        if (nowMs - fpsTimer >= 1000) {
            currentFPS = refreshCount * 1000.0f / (nowMs - fpsTimer);
            refreshCount = 0;
            fpsTimer = nowMs;
        }
    }
}

// --- Diagnostics ---

float CubeEngine::getFPS() {
    return currentFPS;
}

uint32_t CubeEngine::getRefreshCount() {
    return refreshCount;
}

// --- Internal Hardware Functions ---

void CubeEngine::shiftOut16(uint16_t data) {
    /*
     * Sends 16 bits to two cascaded 74HC595 shift registers.
     * 
     * Wiring: ESP → first 74HC595 (handles bits 8-15)
     *         first 74HC595 Q7' → second 74HC595 SER (handles bits 0-7)
     * 
     * We shift MSB first. The first 8 bits shifted will end up in the 
     * second register (they get pushed through), and the last 8 bits 
     * stay in the first register.
     * 
     * After all 16 bits are shifted, we pulse the latch pin to transfer
     * the shift register contents to the output storage registers.
     * 
     * The ULN2803 Darlington array connected to the 595 outputs will
     * INVERT and AMPLIFY the signal to sink LED current.
     */
    
    // Pull latch LOW to begin data transfer
    digitalWrite(LATCH_PIN, LOW);
    
    // Shift out HIGH byte first (goes to second 595 after full transfer)
    // Then LOW byte (stays in first 595)
    // Using direct bit-banging for speed on ESP8266
    for (int8_t i = 15; i >= 0; i--) {
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, (data >> i) & 1);
        digitalWrite(CLOCK_PIN, HIGH);
    }
    
    // Pulse latch to transfer shift register → output register
    digitalWrite(LATCH_PIN, HIGH);
}

void CubeEngine::activateLayer(uint8_t layer) {
    if (layer >= CUBE_Z) return;
    
    // Deactivate all layers first
    for (uint8_t i = 0; i < CUBE_Z; i++) {
        digitalWrite(layerPins[i], HIGH);  // PNP OFF
    }
    
    // Activate the target layer  
    // PNP transistor: pin LOW → transistor conducts → layer anode = VCC
    digitalWrite(layerPins[layer], LOW);
}

void CubeEngine::deactivateAllLayers() {
    for (uint8_t i = 0; i < CUBE_Z; i++) {
        digitalWrite(layerPins[i], HIGH);  // PNP OFF
    }
}
