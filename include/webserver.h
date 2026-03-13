/*
 * webserver.h - Web Server & WebSocket Manager
 * 
 * Uses ESP8266WebServer (built-in) for HTTP serving.
 * Uses WebSocketsServer (links2004) for real-time bidirectional control.
 * Serves static files from LittleFS (HTML, CSS, JS).
 * Provides system info API endpoint.
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "cube_engine.h"
#include "animations.h"

class WebServerManager {
public:
    WebServerManager(CubeEngine& cube, AnimationEngine& anim);
    
    void begin();
    void loop();   // Must be called in main loop
    
    // Broadcast cube state to all WebSocket clients
    void broadcastCubeState();
    void broadcastStatus();

private:
    CubeEngine& cube;
    AnimationEngine& anim;
    
    ESP8266WebServer server;
    WebSocketsServer webSocket;
    
    unsigned long lastBroadcast;
    unsigned long broadcastInterval;
    
    // --- Handlers ---
    void setupRoutes();
    void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    void processMessage(uint8_t clientNum, uint8_t* data, size_t len);
    
    // Send JSON to a specific client
    void sendStatus(uint8_t clientNum);
    void sendCubeState(uint8_t clientNum);
    void sendAnimationList(uint8_t clientNum);
};

#endif // WEBSERVER_H
