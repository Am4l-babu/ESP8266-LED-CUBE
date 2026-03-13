/*
 * animations.h - Animation System for 4x4x4 LED Cube
 * 
 * Provides 20 different animation effects, each with adjustable:
 *   - Speed (animation step interval in ms)
 *   - Brightness (0-255)
 *   - Direction (0-5 for 6 possible directions)
 *   - Density (for particle effects, 1-16)
 * 
 * All animations are NON-BLOCKING using millis() timing.
 * The AnimationEngine manages switching, autoplay, and random mode.
 */

#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include "cube_engine.h"

// --- Animation Types ---
enum AnimationType {
    ANIM_RAIN = 0,
    ANIM_WAVE,
    ANIM_SPIRAL,
    ANIM_SPARKLE,
    ANIM_EXPAND_CUBE,
    ANIM_SHRINK_CUBE,
    ANIM_SNAKE,
    ANIM_FIRE,
    ANIM_RIPPLE,
    ANIM_PLANE_SWEEP,
    ANIM_VOXEL_BOUNCE,
    ANIM_RANDOM_VOXEL,
    ANIM_ROTATING_PLANES,
    ANIM_KNIGHT_RIDER,
    ANIM_ORBITING_POINT,
    ANIM_CUBE_EXPLOSION,
    ANIM_HEARTBEAT,
    ANIM_BREATHING,
    ANIM_FALLING_SAND,
    ANIM_LIGHTNING,
    ANIM_COUNT  // Total number of animations
};

// Direction constants
enum Direction {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_FORWARD,
    DIR_BACKWARD
};

// Animation names for web UI
const char* const ANIM_NAMES[] = {
    "Rain", "Wave", "Spiral", "Sparkle",
    "Expand Cube", "Shrink Cube", "Snake", "Fire",
    "Ripple", "Plane Sweep", "Voxel Bounce", "Random Voxel",
    "Rotating Planes", "Knight Rider", "Orbiting Point", "Cube Explosion",
    "Heartbeat", "Breathing", "Falling Sand", "Lightning"
};

// --- Animation Parameters ---
struct AnimationParams {
    uint16_t speed;       // Step interval in ms (20-2000)
    uint8_t brightness;   // LED brightness (10-255)
    uint8_t direction;    // Direction enum
    uint8_t density;      // Particle density (1-16)
    uint32_t duration;    // Duration in ms (0 = infinite)
};

// --- Animation Engine ---
class AnimationEngine {
public:
    AnimationEngine(CubeEngine& cube);
    
    void begin();
    void update();  // Call in loop() - non-blocking
    
    // --- Animation Control ---
    void setAnimation(AnimationType anim);
    AnimationType getAnimation();
    void nextAnimation();
    void prevAnimation();
    
    // --- Playback Control ---
    void play();
    void pause();
    void stop();
    bool isPlaying();
    
    // --- Autoplay ---
    void setAutoplay(bool enabled, uint32_t interval = 30000);
    bool isAutoplay();
    void setRandomMode(bool enabled);
    bool isRandomMode();
    
    // --- Parameters ---
    void setSpeed(uint16_t speed);
    uint16_t getSpeed();
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness();
    void setDirection(uint8_t dir);
    uint8_t getDirection();
    void setDensity(uint8_t density);
    uint8_t getDensity();
    void setDuration(uint32_t duration);
    AnimationParams getParams();
    
    // --- Pattern Storage ---
    void savePattern(uint8_t slot);
    void loadPattern(uint8_t slot);
    uint8_t getPatternCount();

private:
    CubeEngine& cube;
    
    AnimationType currentAnim;
    AnimationParams params;
    
    bool playing;
    bool autoplay;
    bool randomMode;
    uint32_t autoplayInterval;
    
    // Timing
    unsigned long lastStep;
    unsigned long animStartTime;
    unsigned long lastAutoplaySwitch;
    
    // Animation state variables
    uint8_t step;
    uint8_t phase;
    int8_t pos;
    int8_t dirX, dirY, dirZ;
    uint8_t snakeX[64], snakeY[64], snakeZ[64];
    uint8_t snakeLen;
    float angle;
    uint8_t sandGrid[4][4][4];
    
    // --- Animation Implementations ---
    void animRain();
    void animWave();
    void animSpiral();
    void animSparkle();
    void animExpandCube();
    void animShrinkCube();
    void animSnake();
    void animFire();
    void animRipple();
    void animPlaneSweep();
    void animVoxelBounce();
    void animRandomVoxel();
    void animRotatingPlanes();
    void animKnightRider();
    void animOrbitingPoint();
    void animCubeExplosion();
    void animHeartbeat();
    void animBreathing();
    void animFallingSand();
    void animLightning();
    
    // --- Helpers ---
    void resetState();
};

#endif // ANIMATIONS_H
