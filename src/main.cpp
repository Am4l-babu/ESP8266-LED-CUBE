/*
 * main.cpp - 4x4x4 LED Cube Main Firmware
 * 
 * ESP8266 (NodeMCU) firmware for driving a 4x4x4 LED cube with:
 *   - WiFi connectivity (STA mode with AP fallback)
 *   - ESPAsyncWebServer with WebSocket real-time control
 *   - ArduinoOTA for wireless firmware updates
 *   - 20 built-in animation effects
 *   - LittleFS for web asset storage
 * 
 * Hardware:
 *   2x 74HC595 shift registers → 2x ULN2803 → 16 cathode columns
 *   4 PNP transistors → 4 layer anodes
 *   Multiplexed scanning at ~100Hz
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include "cube_engine.h"
#include "animations.h"
#include "webserver.h"
#include "secrets.h"

// --- Global Objects ---
CubeEngine cubeEngine;
AnimationEngine animEngine(cubeEngine);
WebServerManager webServer(cubeEngine, animEngine);

// --- WiFi Connection ---
bool connectWiFi() {
    Serial.printf("\n[WiFi] Connecting to: %s\n", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        // Blink LEDs during connection
        if (attempts % 2 == 0) {
            cubeEngine.fillLayer(attempts % 4);
        } else {
            cubeEngine.clearAll();
        }
        cubeEngine.refresh();
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("[WiFi] ✓ Connected!"));
        Serial.print(F("[WiFi] IP Address: "));
        Serial.println(WiFi.localIP());
        Serial.print(F("[WiFi] Signal: "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));
        return true;
    }
    
    return false;
}

void startAPMode() {
    Serial.println(F("[WiFi] ✗ Connection failed. Starting AP mode..."));
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.print(F("[WiFi] AP SSID: "));
    Serial.println(AP_SSID);
    Serial.print(F("[WiFi] AP IP: "));
    Serial.println(WiFi.softAPIP());
}

// --- ArduinoOTA Setup ---
void setupOTA() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        Serial.println("[OTA] Start updating " + type);
        // Stop animations during OTA
        cubeEngine.clearAll();
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[OTA] Update complete!"));
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        uint8_t percent = progress / (total / 100);
        Serial.printf("[OTA] Progress: %u%%\r", percent);
        
        // Show progress on cube
        uint8_t ledsOn = (percent * 16) / 100;
        uint16_t pattern = (1 << ledsOn) - 1;
        for (uint8_t z = 0; z < CUBE_Z; z++) {
            cubeEngine.setLayer(z, pattern);
        }
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:    Serial.println(F("Auth Failed")); break;
            case OTA_BEGIN_ERROR:   Serial.println(F("Begin Failed")); break;
            case OTA_CONNECT_ERROR: Serial.println(F("Connect Failed")); break;
            case OTA_RECEIVE_ERROR: Serial.println(F("Receive Failed")); break;
            case OTA_END_ERROR:     Serial.println(F("End Failed")); break;
        }
    });
    
    ArduinoOTA.begin();
    Serial.println(F("[OTA] Ready for wireless updates"));
}

// --- Arduino Setup ---
void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println(F("\n===================================="));
    Serial.println(F("  4x4x4 LED Cube Controller v1.0"));
    Serial.println(F("  ESP8266 + 74HC595 + ULN2803"));
    Serial.println(F("====================================\n"));
    
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println(F("[FS] LittleFS mount FAILED! Formatting..."));
        LittleFS.format();
        LittleFS.begin();
    } else {
        Serial.println(F("[FS] LittleFS mounted successfully"));
    }
    
    // Initialize cube engine
    cubeEngine.begin();
    
    // Quick self-test: flash all LEDs
    Serial.println(F("[Test] LED self-test..."));
    cubeEngine.fillAll();
    for (int i = 0; i < 200; i++) { cubeEngine.refresh(); delay(1); }
    cubeEngine.clearAll();
    for (int i = 0; i < 100; i++) { cubeEngine.refresh(); delay(1); }
    
    // Connect to WiFi
    if (!connectWiFi()) {
        startAPMode();
    }
    
    // Setup OTA
    setupOTA();
    
    // Initialize animation engine
    animEngine.begin();
    animEngine.setAnimation(ANIM_RAIN);
    
    // Start web server
    webServer.begin();
    
    Serial.println(F("\n[System] ✓ All systems initialized!"));
    Serial.println(F("[System] Open the web interface to control the cube."));
    Serial.printf("[System] Free heap: %d bytes\n\n", ESP.getFreeHeap());
}

// --- Arduino Main Loop ---
void loop() {
    // 1. Refresh cube display (call as frequently as possible)
    //    This handles the layer multiplexing scan
    cubeEngine.refresh();
    
    // 2. Update current animation (non-blocking)
    animEngine.update();
    
    // 3. Handle OTA updates
    ArduinoOTA.handle();
    
    // 4. Handle web server
    webServer.loop();
    
    // 5. Yield to prevent watchdog reset
    yield();
}
