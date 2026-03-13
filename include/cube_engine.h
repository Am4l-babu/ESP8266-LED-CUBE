/*
 * cube_engine.h - 4x4x4 LED Cube Engine
 * 
 * Hardware Architecture:
 * - 2x 74HC595 shift registers cascade to drive 16 cathode columns
 * - 2x ULN2803 Darlington arrays sink current through LED cathodes
 * - 4 PNP transistors (or P-MOSFETs) switch layer anodes HIGH
 * - Multiplexing: only one layer active at a time, scanned rapidly
 * 
 * Signal Flow:
 *   ESP8266 → 74HC595 (serial data) → ULN2803 (current sink) → LED cathodes
 *   ESP8266 → PNP transistor → LED layer anode (common anode per layer)
 * 
 * Coordinate System:
 *   x = column (0-3, left to right)
 *   y = row (0-3, front to back)  
 *   z = layer (0-3, bottom to top)
 * 
 * Pin Assignments:
 *   DATA_PIN  = D3 (GPIO0)  - Serial data to first 74HC595
 *   CLOCK_PIN = D2 (GPIO4)  - Shift clock for 74HC595 chain
 *   LATCH_PIN = D1 (GPIO5)  - Storage latch for 74HC595 chain
 *   LAYER1-4  = D5-D8       - Layer enable (active LOW for PNP)
 */

#ifndef CUBE_ENGINE_H
#define CUBE_ENGINE_H

#include <Arduino.h>

// --- Pin Definitions ---
#define DATA_PIN   D3    // Serial data output to 74HC595
#define CLOCK_PIN  D2    // Shift register clock
#define LATCH_PIN  D1    // Storage register latch

// Layer control pins (active LOW for PNP transistors)
#define LAYER_PIN_0  D5  // Bottom layer (z=0)
#define LAYER_PIN_1  D6  // Layer z=1
#define LAYER_PIN_2  D7  // Layer z=2
#define LAYER_PIN_3  D8  // Top layer (z=3)

// --- Cube Dimensions ---
#define CUBE_X  4
#define CUBE_Y  4
#define CUBE_Z  4
#define NUM_LEDS (CUBE_X * CUBE_Y * CUBE_Z)

// Refresh timing
#define LAYER_HOLD_US  2500   // Microseconds each layer is active
#define MIN_BRIGHTNESS 10
#define MAX_BRIGHTNESS 255

class CubeEngine {
public:
    CubeEngine();
    
    // --- Initialization ---
    void begin();
    
    // --- Voxel Operations (3D coordinate addressing) ---
    void setVoxel(uint8_t x, uint8_t y, uint8_t z, bool state = true);
    void clearVoxel(uint8_t x, uint8_t y, uint8_t z);
    bool getVoxel(uint8_t x, uint8_t y, uint8_t z);
    void toggleVoxel(uint8_t x, uint8_t y, uint8_t z);
    
    // --- Layer Operations ---
    void setLayer(uint8_t z, uint16_t pattern);
    uint16_t getLayer(uint8_t z);
    void fillLayer(uint8_t z, bool state = true);
    
    // --- Bulk Operations ---
    void clearAll();
    void fillAll();
    void setBuffer(const uint16_t buf[CUBE_Z]);
    void getBuffer(uint16_t buf[CUBE_Z]);
    
    // --- Refresh (call frequently in loop) ---
    void refresh();
    
    // --- Brightness ---
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness();
    
    // --- Diagnostics ---
    float getFPS();
    uint32_t getRefreshCount();

private:
    // Frame buffer: 4 layers, 16 bits each (4x4 = 16 columns per layer)
    // Bit mapping: bit[y*4+x] = column at position (x,y)
    uint16_t frameBuffer[CUBE_Z];
    
    // Current layer being scanned
    uint8_t currentLayer;
    
    // Brightness (0-255)
    uint8_t brightness;
    
    // Layer pin lookup table
    uint8_t layerPins[CUBE_Z];
    
    // FPS tracking
    uint32_t refreshCount;
    uint32_t fpsTimer;
    float currentFPS;
    
    // Timing
    unsigned long lastRefreshMicros;
    
    // --- Internal Hardware Functions ---
    
    // Send 16-bit column data to two cascaded 74HC595 shift registers
    // The first 595 receives the HIGH byte, second receives LOW byte
    // After shifting, the latch pin is pulsed to transfer data to outputs
    // ULN2803 inverts the output: HIGH from 595 → current sink ON → LED ON
    void shiftOut16(uint16_t data);
    
    // Activate a specific layer (set its PNP transistor LOW)
    // All other layers are deactivated (set HIGH)
    void activateLayer(uint8_t layer);
    
    // Deactivate all layers (all PNP transistors OFF)
    void deactivateAllLayers();
};

#endif // CUBE_ENGINE_H
