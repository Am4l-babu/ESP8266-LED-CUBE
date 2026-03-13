/*
 * animations.cpp - 20 LED Cube Animation Effects
 * 
 * All animations are non-blocking, using millis() for timing.
 * Each animation function is called repeatedly from update().
 * The 'step' counter tracks the animation progress and resets when the
 * animation cycles back to the beginning.
 */

#include "animations.h"
#include <LittleFS.h>

// --- Constructor ---
AnimationEngine::AnimationEngine(CubeEngine& cubeRef) : cube(cubeRef) {
    currentAnim = ANIM_RAIN;
    params.speed = 100;
    params.brightness = 200;
    params.direction = DIR_DOWN;
    params.density = 4;
    params.duration = 0;
    
    playing = true;
    autoplay = false;
    randomMode = false;
    autoplayInterval = 30000;
    
    lastStep = 0;
    animStartTime = 0;
    lastAutoplaySwitch = 0;
    
    resetState();
}

void AnimationEngine::begin() {
    Serial.println(F("[AnimEngine] Initialized with 20 animations"));
    animStartTime = millis();
    lastAutoplaySwitch = millis();
}

void AnimationEngine::resetState() {
    step = 0;
    phase = 0;
    pos = 0;
    dirX = 1; dirY = 0; dirZ = 0;
    snakeLen = 1;
    snakeX[0] = 0; snakeY[0] = 0; snakeZ[0] = 0;
    angle = 0;
    memset(sandGrid, 0, sizeof(sandGrid));
}

// --- Main Update (NON-BLOCKING) ---
void AnimationEngine::update() {
    if (!playing) return;
    
    unsigned long now = millis();
    
    // Check duration limit
    if (params.duration > 0 && (now - animStartTime) >= params.duration) {
        if (autoplay) {
            if (randomMode) {
                setAnimation((AnimationType)random(ANIM_COUNT));
            } else {
                nextAnimation();
            }
        } else {
            stop();
        }
        return;
    }
    
    // Autoplay switching
    if (autoplay && (now - lastAutoplaySwitch) >= autoplayInterval) {
        lastAutoplaySwitch = now;
        if (randomMode) {
            setAnimation((AnimationType)random(ANIM_COUNT));
        } else {
            nextAnimation();
        }
        return;
    }
    
    // Check if it's time for the next animation step
    if (now - lastStep < params.speed) return;
    lastStep = now;
    
    // Apply brightness
    cube.setBrightness(params.brightness);
    
    // Run current animation
    switch (currentAnim) {
        case ANIM_RAIN:            animRain(); break;
        case ANIM_WAVE:            animWave(); break;
        case ANIM_SPIRAL:          animSpiral(); break;
        case ANIM_SPARKLE:         animSparkle(); break;
        case ANIM_EXPAND_CUBE:     animExpandCube(); break;
        case ANIM_SHRINK_CUBE:     animShrinkCube(); break;
        case ANIM_SNAKE:           animSnake(); break;
        case ANIM_FIRE:            animFire(); break;
        case ANIM_RIPPLE:          animRipple(); break;
        case ANIM_PLANE_SWEEP:     animPlaneSweep(); break;
        case ANIM_VOXEL_BOUNCE:    animVoxelBounce(); break;
        case ANIM_RANDOM_VOXEL:    animRandomVoxel(); break;
        case ANIM_ROTATING_PLANES: animRotatingPlanes(); break;
        case ANIM_KNIGHT_RIDER:    animKnightRider(); break;
        case ANIM_ORBITING_POINT:  animOrbitingPoint(); break;
        case ANIM_CUBE_EXPLOSION:  animCubeExplosion(); break;
        case ANIM_HEARTBEAT:       animHeartbeat(); break;
        case ANIM_BREATHING:       animBreathing(); break;
        case ANIM_FALLING_SAND:    animFallingSand(); break;
        case ANIM_LIGHTNING:       animLightning(); break;
        default: break;
    }
}

// --- Animation Control ---

void AnimationEngine::setAnimation(AnimationType anim) {
    if (anim >= ANIM_COUNT) anim = ANIM_RAIN;
    currentAnim = anim;
    resetState();
    cube.clearAll();
    animStartTime = millis();
    playing = true;
    Serial.printf("[AnimEngine] Animation: %s\n", ANIM_NAMES[anim]);
}

AnimationType AnimationEngine::getAnimation() { return currentAnim; }

void AnimationEngine::nextAnimation() {
    setAnimation((AnimationType)((currentAnim + 1) % ANIM_COUNT));
}

void AnimationEngine::prevAnimation() {
    setAnimation((AnimationType)((currentAnim + ANIM_COUNT - 1) % ANIM_COUNT));
}

// --- Playback ---

void AnimationEngine::play() { playing = true; animStartTime = millis(); }
void AnimationEngine::pause() { playing = false; }
void AnimationEngine::stop() { playing = false; cube.clearAll(); resetState(); }
bool AnimationEngine::isPlaying() { return playing; }

// --- Autoplay ---

void AnimationEngine::setAutoplay(bool enabled, uint32_t interval) {
    autoplay = enabled;
    autoplayInterval = interval;
    lastAutoplaySwitch = millis();
}

bool AnimationEngine::isAutoplay() { return autoplay; }
void AnimationEngine::setRandomMode(bool enabled) { randomMode = enabled; }
bool AnimationEngine::isRandomMode() { return randomMode; }

// --- Parameters ---

void AnimationEngine::setSpeed(uint16_t speed) { params.speed = constrain(speed, 20, 2000); }
uint16_t AnimationEngine::getSpeed() { return params.speed; }

void AnimationEngine::setBrightness(uint8_t b) {
    params.brightness = constrain(b, 10, 255);
    cube.setBrightness(params.brightness);
}
uint8_t AnimationEngine::getBrightness() { return params.brightness; }

void AnimationEngine::setDirection(uint8_t dir) { params.direction = dir % 6; }
uint8_t AnimationEngine::getDirection() { return params.direction; }

void AnimationEngine::setDensity(uint8_t d) { params.density = constrain(d, 1, 16); }
uint8_t AnimationEngine::getDensity() { return params.density; }

void AnimationEngine::setDuration(uint32_t duration) { params.duration = duration; }
AnimationParams AnimationEngine::getParams() { return params; }

// --- Pattern Storage (LittleFS) ---

void AnimationEngine::savePattern(uint8_t slot) {
    if (slot >= 10) return;
    
    String filename = "/patterns/slot" + String(slot) + ".bin";
    
    // Ensure directory exists
    if (!LittleFS.exists("/patterns")) {
        LittleFS.mkdir("/patterns");
    }
    
    File f = LittleFS.open(filename, "w");
    if (!f) { Serial.println(F("[AnimEngine] Pattern save failed")); return; }
    
    uint16_t buf[CUBE_Z];
    cube.getBuffer(buf);
    f.write((uint8_t*)buf, sizeof(buf));
    f.write((uint8_t*)&currentAnim, sizeof(currentAnim));
    f.write((uint8_t*)&params, sizeof(params));
    f.close();
    
    Serial.printf("[AnimEngine] Pattern saved to slot %d\n", slot);
}

void AnimationEngine::loadPattern(uint8_t slot) {
    if (slot >= 10) return;
    
    String filename = "/patterns/slot" + String(slot) + ".bin";
    
    File f = LittleFS.open(filename, "r");
    if (!f) { Serial.println(F("[AnimEngine] Pattern not found")); return; }
    
    uint16_t buf[CUBE_Z];
    f.read((uint8_t*)buf, sizeof(buf));
    cube.setBuffer(buf);
    
    f.read((uint8_t*)&currentAnim, sizeof(currentAnim));
    f.read((uint8_t*)&params, sizeof(params));
    f.close();
    
    Serial.printf("[AnimEngine] Pattern loaded from slot %d\n", slot);
}

uint8_t AnimationEngine::getPatternCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < 10; i++) {
        String filename = "/patterns/slot" + String(i) + ".bin";
        if (LittleFS.exists(filename)) count++;
    }
    return count;
}

// ========================================
// ANIMATION IMPLEMENTATIONS
// ========================================

// --- RAIN: Drops fall from top to bottom ---
void AnimationEngine::animRain() {
    // Shift all layers down
    for (uint8_t z = 0; z < CUBE_Z - 1; z++) {
        cube.setLayer(z, cube.getLayer(z + 1));
    }
    
    // Generate new random drops on top layer
    uint16_t topLayer = 0;
    for (uint8_t i = 0; i < params.density; i++) {
        if (random(100) < 30) {
            topLayer |= (1 << random(16));
        }
    }
    cube.setLayer(CUBE_Z - 1, topLayer);
}

// --- WAVE: Sine wave sweeps across cube ---
void AnimationEngine::animWave() {
    cube.clearAll();
    
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            float dist = sqrt((float)(x * x + y * y));
            int z = (int)(1.5 + 1.5 * sin(angle + dist * 0.8));
            z = constrain(z, 0, CUBE_Z - 1);
            cube.setVoxel(x, y, z);
        }
    }
    angle += 0.3;
    if (angle > 2 * PI) angle -= 2 * PI;
}

// --- SPIRAL: LEDs spiral up the cube ---
void AnimationEngine::animSpiral() {
    cube.clearAll();
    
    // Spiral path coordinates for one layer
    const uint8_t spiralX[] = {0,1,2,3, 3,3,3, 2,1,0, 0,0, 1,2, 2, 1};
    const uint8_t spiralY[] = {0,0,0,0, 1,2,3, 3,3,3, 2,1, 1,1, 2, 2};
    const uint8_t spiralLen = 16;
    
    // Light up a segment of the spiral at current position
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t idx = (step + i) % spiralLen;
        uint8_t z = ((step + i) / spiralLen) % CUBE_Z;
        cube.setVoxel(spiralX[idx], spiralY[idx], z);
    }
    
    step++;
    if (step >= spiralLen * CUBE_Z) step = 0;
}

// --- SPARKLE: Random LEDs flash on and off ---
void AnimationEngine::animSparkle() {
    cube.clearAll();
    
    for (uint8_t i = 0; i < params.density; i++) {
        cube.setVoxel(random(CUBE_X), random(CUBE_Y), random(CUBE_Z));
    }
}

// --- EXPANDING CUBE: Small cube grows to fill entire cube ---
void AnimationEngine::animExpandCube() {
    cube.clearAll();
    
    uint8_t size = (step % 4);  // 0=1x1, 1=2x2, 2=3x3, 3=4x4
    uint8_t start = (3 - size) / 2;
    uint8_t end = start + size;
    
    for (uint8_t x = start; x <= end; x++) {
        for (uint8_t y = start; y <= end; y++) {
            for (uint8_t z = start; z <= end; z++) {
                // Draw edges only for hollow cube effect
                bool isEdge = (x == start || x == end || 
                              y == start || y == end || 
                              z == start || z == end);
                if (isEdge) cube.setVoxel(x, y, z);
            }
        }
    }
    
    step++;
    if (step >= 8) step = 0;  // Expand then contract
}

// --- SHRINKING CUBE: Full cube shrinks to center ---
void AnimationEngine::animShrinkCube() {
    cube.clearAll();
    
    uint8_t size = 3 - (step % 4);
    uint8_t start = (3 - size) / 2;
    uint8_t end = start + size;
    
    for (uint8_t x = start; x <= end; x++) {
        for (uint8_t y = start; y <= end; y++) {
            for (uint8_t z = start; z <= end; z++) {
                bool isEdge = (x == start || x == end || 
                              y == start || y == end || 
                              z == start || z == end);
                if (isEdge) cube.setVoxel(x, y, z);
            }
        }
    }
    
    step++;
    if (step >= 8) step = 0;
}

// --- SNAKE: A snake moves randomly through the cube ---
void AnimationEngine::animSnake() {
    // Try random direction
    int8_t nx = snakeX[0], ny = snakeY[0], nz = snakeZ[0];
    uint8_t dir = random(6);
    
    switch (dir) {
        case 0: nx++; break;
        case 1: nx--; break;
        case 2: ny++; break;
        case 3: ny--; break;
        case 4: nz++; break;
        case 5: nz--; break;
    }
    
    // Wrap around
    nx = (nx + CUBE_X) % CUBE_X;
    ny = (ny + CUBE_Y) % CUBE_Y;
    nz = (nz + CUBE_Z) % CUBE_Z;
    
    // Move body
    uint8_t maxLen = constrain(params.density, 2, 16);
    if (snakeLen < maxLen) snakeLen++;
    
    for (int8_t i = snakeLen - 1; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
        snakeZ[i] = snakeZ[i-1];
    }
    snakeX[0] = nx;
    snakeY[0] = ny;
    snakeZ[0] = nz;
    
    // Draw snake
    cube.clearAll();
    for (uint8_t i = 0; i < snakeLen; i++) {
        cube.setVoxel(snakeX[i], snakeY[i], snakeZ[i]);
    }
}

// --- FIRE: Fire rises from bottom ---
void AnimationEngine::animFire() {
    // Shift up
    for (int8_t z = CUBE_Z - 1; z > 0; z--) {
        uint16_t belowLayer = cube.getLayer(z - 1);
        // Random decay as fire rises
        uint16_t decayed = 0;
        for (uint8_t bit = 0; bit < 16; bit++) {
            if ((belowLayer >> bit) & 1) {
                if (random(100) < 60) { // 60% chance to survive
                    decayed |= (1 << bit);
                }
            }
        }
        cube.setLayer(z, decayed);
    }
    
    // New fire base
    uint16_t base = 0;
    for (uint8_t i = 0; i < 16; i++) {
        if (random(100) < params.density * 6) {
            base |= (1 << i);
        }
    }
    cube.setLayer(0, base);
}

// --- RIPPLE: Concentric ripple from center ---
void AnimationEngine::animRipple() {
    cube.clearAll();
    
    float center = 1.5;
    
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            float dist = sqrt(pow(x - center, 2) + pow(y - center, 2));
            float wave = sin(dist * 2.0 - angle);
            if (wave > 0.3) {
                int z = (int)(center + center * wave * 0.5);
                z = constrain(z, 0, CUBE_Z - 1);
                cube.setVoxel(x, y, z);
            }
        }
    }
    angle += 0.4;
    if (angle > 2 * PI) angle -= 2 * PI;
}

// --- PLANE SWEEP: A plane sweeps across an axis ---
void AnimationEngine::animPlaneSweep() {
    cube.clearAll();
    
    uint8_t p = step % (CUBE_Z * 2);  // Ping-pong
    uint8_t layer = (p < CUBE_Z) ? p : (CUBE_Z * 2 - 1 - p);
    
    switch (params.direction % 3) {
        case 0:  // Z-axis sweep
            cube.fillLayer(layer);
            break;
        case 1:  // X-axis sweep
            for (uint8_t y = 0; y < CUBE_Y; y++)
                for (uint8_t z = 0; z < CUBE_Z; z++)
                    cube.setVoxel(layer, y, z);
            break;
        case 2:  // Y-axis sweep
            for (uint8_t x = 0; x < CUBE_X; x++)
                for (uint8_t z = 0; z < CUBE_Z; z++)
                    cube.setVoxel(x, layer, z);
            break;
    }
    
    step++;
}

// --- VOXEL BOUNCE: A single voxel bounces around ---
void AnimationEngine::animVoxelBounce() {
    cube.clearAll();
    
    cube.setVoxel(snakeX[0], snakeY[0], snakeZ[0]);
    
    // Move
    snakeX[0] += dirX;
    snakeY[0] += dirY;
    snakeZ[0] += dirZ;
    
    // Bounce off walls
    if (snakeX[0] >= CUBE_X || snakeX[0] > 200) { snakeX[0] = (dirX > 0) ? CUBE_X - 1 : 0; dirX = -dirX; }
    if (snakeY[0] >= CUBE_Y || snakeY[0] > 200) { snakeY[0] = (dirY > 0) ? CUBE_Y - 1 : 0; dirY = -dirY; }
    if (snakeZ[0] >= CUBE_Z || snakeZ[0] > 200) { snakeZ[0] = (dirZ > 0) ? CUBE_Z - 1 : 0; dirZ = -dirZ; }
    
    // Random direction change
    if (random(100) < 20) {
        uint8_t axis = random(3);
        if (axis == 0) dirX = (dirX == 0) ? (random(2) ? 1 : -1) : 0;
        if (axis == 1) dirY = (dirY == 0) ? (random(2) ? 1 : -1) : 0;
        if (axis == 2) dirZ = (dirZ == 0) ? (random(2) ? 1 : -1) : 0;
    }
    
    // Ensure at least one direction
    if (dirX == 0 && dirY == 0 && dirZ == 0) dirX = 1;
}

// --- RANDOM VOXEL: Random voxels turn on one by one ---
void AnimationEngine::animRandomVoxel() {
    if (step < 64) {
        cube.setVoxel(random(CUBE_X), random(CUBE_Y), random(CUBE_Z));
        step++;
    } else {
        cube.clearAll();
        step = 0;
    }
}

// --- ROTATING PLANES: A plane rotates around the center ---
void AnimationEngine::animRotatingPlanes() {
    cube.clearAll();
    
    float cx = 1.5, cy = 1.5;
    
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            float dx = x - cx;
            float dy = y - cy;
            float rotated = dx * cos(angle) + dy * sin(angle);
            
            if (fabs(rotated) < 0.8) {
                for (uint8_t z = 0; z < CUBE_Z; z++) {
                    cube.setVoxel(x, y, z);
                }
            }
        }
    }
    
    angle += 0.15;
    if (angle > 2 * PI) angle -= 2 * PI;
}

// --- KNIGHT RIDER: Scanning bar moves back and forth ---
void AnimationEngine::animKnightRider() {
    cube.clearAll();
    
    uint8_t p = step % (CUBE_X * 2 - 2);
    uint8_t col = (p < CUBE_X) ? p : (CUBE_X * 2 - 2 - p);
    
    for (uint8_t y = 0; y < CUBE_Y; y++) {
        for (uint8_t z = 0; z < CUBE_Z; z++) {
            cube.setVoxel(col, y, z);
            // Trail effect
            if (col > 0) cube.setVoxel(col - 1, y, z);
        }
    }
    
    step++;
}

// --- ORBITING POINT: A point orbits the center ---
void AnimationEngine::animOrbitingPoint() {
    cube.clearAll();
    
    float cx = 1.5, cy = 1.5, cz = 1.5;
    float radius = 1.5;
    
    uint8_t x = constrain((int)(cx + radius * cos(angle)), 0, 3);
    uint8_t y = constrain((int)(cy + radius * sin(angle)), 0, 3);
    uint8_t z = constrain((int)(cz + radius * sin(angle * 0.7)), 0, 3);
    
    cube.setVoxel(x, y, z);
    
    // Trail
    uint8_t x2 = constrain((int)(cx + radius * cos(angle - 0.5)), 0, 3);
    uint8_t y2 = constrain((int)(cy + radius * sin(angle - 0.5)), 0, 3);
    uint8_t z2 = constrain((int)(cz + radius * sin((angle - 0.5) * 0.7)), 0, 3);
    cube.setVoxel(x2, y2, z2);
    
    angle += 0.2;
    if (angle > 2 * PI) angle -= 2 * PI;
}

// --- CUBE EXPLOSION: Center expands outward then resets ---
void AnimationEngine::animCubeExplosion() {
    cube.clearAll();
    
    if (phase == 0) {
        // Expanding phase
        float t = step / 8.0;
        for (int i = 0; i < 8; i++) {
            float ax = (i & 1) ? 1.0 : -1.0;
            float ay = (i & 2) ? 1.0 : -1.0;
            float az = (i & 4) ? 1.0 : -1.0;
            
            int x = constrain((int)(1.5 + ax * t), 0, 3);
            int y = constrain((int)(1.5 + ay * t), 0, 3);
            int z = constrain((int)(1.5 + az * t), 0, 3);
            cube.setVoxel(x, y, z);
        }
        
        step++;
        if (step >= 12) { step = 0; phase = 1; }
    } else {
        // Pause then reset
        step++;
        if (step >= 4) { step = 0; phase = 0; }
    }
}

// --- HEARTBEAT: Pulsing pattern like a heartbeat ---
void AnimationEngine::animHeartbeat() {
    cube.clearAll();
    
    // Double-beat pattern
    const uint8_t pattern[] = {0, 2, 3, 2, 0, 0, 1, 2, 1, 0, 0, 0};
    uint8_t beatPhase = step % 12;
    uint8_t intensity = pattern[beatPhase];
    
    // Draw concentric fill based on intensity
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            for (uint8_t z = 0; z < CUBE_Z; z++) {
                float dist = sqrt(pow(x - 1.5, 2) + pow(y - 1.5, 2) + pow(z - 1.5, 2));
                if (dist <= intensity) {
                    cube.setVoxel(x, y, z);
                }
            }
        }
    }
    
    step++;
}

// --- BREATHING: Slow fade in/out of all LEDs ---
void AnimationEngine::animBreathing() {
    // Use sine wave for smooth brightness transition
    float brightness_f = (sin(angle) + 1.0) / 2.0;
    uint8_t b = (uint8_t)(brightness_f * params.brightness);
    cube.setBrightness(max((uint8_t)10, b));
    
    // Fill all when breathing
    cube.fillAll();
    
    angle += 0.05;
    if (angle > 2 * PI) angle -= 2 * PI;
}

// --- FALLING SAND: Particles fall and stack up ---
void AnimationEngine::animFallingSand() {
    // Add new particles at top
    if (random(100) < params.density * 8) {
        uint8_t x = random(CUBE_X);
        uint8_t y = random(CUBE_Y);
        if (!sandGrid[x][y][CUBE_Z - 1]) {
            sandGrid[x][y][CUBE_Z - 1] = 1;
        }
    }
    
    // Simulate gravity - move particles down
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            for (uint8_t z = 0; z < CUBE_Z - 1; z++) {
                if (!sandGrid[x][y][z] && sandGrid[x][y][z + 1]) {
                    sandGrid[x][y][z] = 1;
                    sandGrid[x][y][z + 1] = 0;
                }
            }
        }
    }
    
    // Render
    cube.clearAll();
    for (uint8_t x = 0; x < CUBE_X; x++) {
        for (uint8_t y = 0; y < CUBE_Y; y++) {
            for (uint8_t z = 0; z < CUBE_Z; z++) {
                if (sandGrid[x][y][z]) cube.setVoxel(x, y, z);
            }
        }
    }
    
    // Check if full, then clear
    step++;
    if (step > 60) {
        memset(sandGrid, 0, sizeof(sandGrid));
        step = 0;
    }
}

// --- LIGHTNING: Random lightning bolt flashes ---
void AnimationEngine::animLightning() {
    if (phase == 0) {
        // Flash phase
        cube.clearAll();
        
        // Random lightning path from top to bottom
        uint8_t x = random(CUBE_X);
        uint8_t y = random(CUBE_Y);
        
        for (int8_t z = CUBE_Z - 1; z >= 0; z--) {
            cube.setVoxel(x, y, z);
            // Random branch
            if (random(100) < 40) {
                uint8_t bx = constrain(x + random(3) - 1, 0, 3);
                uint8_t by = constrain(y + random(3) - 1, 0, 3);
                cube.setVoxel(bx, by, z);
            }
            // Drift
            x = constrain(x + random(3) - 1, 0, 3);
            y = constrain(y + random(3) - 1, 0, 3);
        }
        
        phase = 1;
        step = 0;
    } else {
        // Dark pause between strikes
        step++;
        if (step > 2) {
            cube.clearAll();
        }
        if (step > (uint8_t)(8 - params.density / 2)) {
            phase = 0;
        }
    }
}
