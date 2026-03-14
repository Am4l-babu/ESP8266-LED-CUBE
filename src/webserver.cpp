/*
 * webserver.cpp - ESP8266WebServer + WebSocketsServer Implementation
 * 
 * HTTP Server:
 *   - Serves static files from LittleFS (index.html, styles.css, JS files)
 *   - Provides /api/status JSON endpoint for system info
 * 
 * WebSocket Protocol (JSON) at port 81:
 *   Client → Server:
 *     { "cmd": "setAnim",    "value": 0-19 }
 *     { "cmd": "play" }
 *     { "cmd": "pause" }
 *     { "cmd": "stop" }
 *     { "cmd": "next" }
 *     { "cmd": "prev" }
 *     { "cmd": "speed",      "value": 20-2000 }
 *     { "cmd": "brightness", "value": 10-255 }
 *     { "cmd": "direction",  "value": 0-5 }
 *     { "cmd": "density",    "value": 1-16 }
 *     { "cmd": "autoplay",   "value": true/false }
 *     { "cmd": "random",     "value": true/false }
 *     { "cmd": "setVoxel",   "x":0-3, "y":0-3, "z":0-3, "state": true/false }
 *     { "cmd": "clearAll" }
 *     { "cmd": "fillAll" }
 *     { "cmd": "savePattern","slot": 0-9 }
 *     { "cmd": "loadPattern","slot": 0-9 }
 *     { "cmd": "getState" }
 *     { "cmd": "getAnims" }
 *     { "cmd": "setBuffer",  "layers": [0xFFFF, 0x0000, 0x0000, 0x0000] }
 * 
 *   Server → Client:
 *     { "type": "status",    ... parameters ... }
 *     { "type": "cubeState", "layers": [...] }
 *     { "type": "animList",  "anims": [...] }
 */

#include "webserver.h"
#include <LittleFS.h>

WebServerManager::WebServerManager(CubeEngine& cubeRef, AnimationEngine& animRef, MusicController& musicRef)
    : cube(cubeRef), anim(animRef), music(musicRef), server(80), webSocket(81) {
    lastBroadcast = 0;
    broadcastInterval = 200;  // Broadcast state every 200ms
}

void WebServerManager::begin() {
    // Setup WebSocket handler
    webSocket.begin();
    webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        this->handleWebSocketEvent(num, type, payload, length);
    });
    
    // Setup HTTP routes
    setupRoutes();
    
    // Start HTTP server
    server.begin();
    
    Serial.println(F("[WebServer] HTTP server on port 80"));
    Serial.println(F("[WebServer] WebSocket server on port 81"));
}

// Helper: get MIME type from file extension
static String getMimeType(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png"))  return "image/png";
    if (path.endsWith(".jpg"))  return "image/jpeg";
    if (path.endsWith(".ico"))  return "image/x-icon";
    if (path.endsWith(".svg"))  return "image/svg+xml";
    return "text/plain";
}

// Helper: serve a file from LittleFS
static bool serveFile(ESP8266WebServer& server, const String& path) {
    String filePath = path;
    if (filePath.endsWith("/")) filePath += "index.html";
    
    if (LittleFS.exists(filePath)) {
        File file = LittleFS.open(filePath, "r");
        if (file) {
            server.streamFile(file, getMimeType(filePath));
            file.close();
            return true;
        }
    }
    return false;
}

void WebServerManager::setupRoutes() {
    // Serve index.html at root
    server.on("/", HTTP_GET, [this]() {
        if (!serveFile(this->server, "/index.html")) {
            this->server.send(404, "text/plain", "File not found");
        }
    });
    
    // API: System status
    server.on("/api/status", HTTP_GET, [this]() {
        StaticJsonDocument<512> doc;
        doc["heap"] = ESP.getFreeHeap();
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = millis() / 1000;
        doc["fps"] = cube.getFPS();
        doc["version"] = LED_CUBE_VERSION;
        doc["animation"] = ANIM_NAMES[anim.getAnimation()];
        doc["animIndex"] = anim.getAnimation();
        doc["playing"] = anim.isPlaying();
        doc["autoplay"] = anim.isAutoplay();
        doc["randomMode"] = anim.isRandomMode();
        doc["speed"] = anim.getSpeed();
        doc["brightness"] = anim.getBrightness();
        doc["direction"] = anim.getDirection();
        doc["density"] = anim.getDensity();
        doc["patterns"] = anim.getPatternCount();
        doc["ip"] = WiFi.localIP().toString();
        doc["musicMode"] = music.getModeName();
        doc["musicActive"] = music.isActive();
        doc["ledColor"] = anim.getLedColor();
        
        String response;
        serializeJson(doc, response);
        this->server.send(200, "application/json", response);
    });
    
    // API: Per-LED hardware color mapping
    server.on("/api/colors", HTTP_GET, [this]() {
        if (!LittleFS.exists("/colors.json")) {
            // Return default 64 cyan colors
            String defaultColors = "{\"colors\":[";
            for(int i=0; i<64; i++) {
                defaultColors += "\"#00e5ff\"";
                if(i < 63) defaultColors += ",";
            }
            defaultColors += "]}";
            this->server.send(200, "application/json", defaultColors);
            return;
        }
        
        File file = LittleFS.open("/colors.json", "r");
        if (file) {
            this->server.streamFile(file, "application/json");
            file.close();
        } else {
            this->server.send(500, "application/json", "{\"error\":\"read failed\"}");
        }
    });
    
    server.on("/api/colors", HTTP_POST, [this]() {
        if (this->server.hasArg("plain")) {
            String body = this->server.arg("plain");
            File file = LittleFS.open("/colors.json", "w");
            if (file) {
                file.print(body);
                file.close();
                this->server.send(200, "application/json", "{\"status\":\"success\"}");
                
                // Tell clients to reload the color map
                StaticJsonDocument<64> doc;
                doc["type"] = "colorMapUpdate";
                String msg;
                serializeJson(doc, msg);
                this->webSocket.broadcastTXT(msg);
            } else {
                this->server.send(500, "application/json", "{\"error\":\"write failed\"}");
            }
        } else {
            this->server.send(400, "application/json", "{\"error\":\"bad request\"}");
        }
    });
    
    // Serve all other static files from LittleFS
    server.onNotFound([this]() {
        if (!serveFile(this->server, this->server.uri())) {
            this->server.send(404, "text/plain", "Not Found");
        }
    });
}

void WebServerManager::loop() {
    // Handle HTTP requests
    server.handleClient();
    
    // Handle WebSocket events
    webSocket.loop();
    
    // Periodic state broadcast
    unsigned long now = millis();
    if (now - lastBroadcast >= broadcastInterval) {
        lastBroadcast = now;
        if (webSocket.connectedClients() > 0) {
            broadcastCubeState();
        }
    }
}

// --- WebSocket Event Handler ---

void WebServerManager::handleWebSocketEvent(uint8_t num, WStype_t type, 
    uint8_t* payload, size_t length) {
    
    switch (type) {
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[WebSocket] Client #%u connected from %s\n", num, ip.toString().c_str());
            // Send initial state
            sendStatus(num);
            sendAnimationList(num);
            sendCubeState(num);
            break;
        }
            
        case WStype_DISCONNECTED:
            Serial.printf("[WebSocket] Client #%u disconnected\n", num);
            break;
            
        case WStype_TEXT:
            processMessage(num, payload, length);
            break;
        
        default:
            break;
    }
}

// --- Process Incoming WebSocket Messages ---

void WebServerManager::processMessage(uint8_t clientNum, uint8_t* data, size_t len) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, (char*)data, len);
    
    if (error) {
        Serial.printf("[WebSocket] JSON parse error: %s\n", error.c_str());
        return;
    }
    
    const char* cmd = doc["cmd"];
    if (!cmd) return;
    
    // --- Animation Control ---
    if (strcmp(cmd, "setAnim") == 0) {
        int val = doc["value"] | 0;
        anim.setAnimation((AnimationType)val);
    }
    else if (strcmp(cmd, "play") == 0) {
        anim.play();
    }
    else if (strcmp(cmd, "pause") == 0) {
        anim.pause();
    }
    else if (strcmp(cmd, "stop") == 0) {
        anim.stop();
    }
    else if (strcmp(cmd, "next") == 0) {
        anim.nextAnimation();
    }
    else if (strcmp(cmd, "prev") == 0) {
        anim.prevAnimation();
    }
    // --- Parameters ---
    else if (strcmp(cmd, "speed") == 0) {
        anim.setSpeed(doc["value"] | 100);
    }
    else if (strcmp(cmd, "brightness") == 0) {
        anim.setBrightness(doc["value"] | 200);
    }
    else if (strcmp(cmd, "direction") == 0) {
        anim.setDirection(doc["value"] | 0);
    }
    else if (strcmp(cmd, "density") == 0) {
        anim.setDensity(doc["value"] | 4);
    }
    // --- Autoplay ---
    else if (strcmp(cmd, "autoplay") == 0) {
        anim.setAutoplay(doc["value"] | false);
    }
    else if (strcmp(cmd, "random") == 0) {
        anim.setRandomMode(doc["value"] | false);
    }
    // --- LED Color ---
    else if (strcmp(cmd, "setColor") == 0) {
        const char* color = doc["value"];
        if (color) anim.setLedColor(color);
    }
    // --- Voxel Control ---
    else if (strcmp(cmd, "setVoxel") == 0) {
        uint8_t x = doc["x"] | 0;
        uint8_t y = doc["y"] | 0;
        uint8_t z = doc["z"] | 0;
        bool state = doc["state"] | true;
        cube.setVoxel(x, y, z, state);
    }
    else if (strcmp(cmd, "clearAll") == 0) {
        anim.stop();
        cube.clearAll();
    }
    else if (strcmp(cmd, "fillAll") == 0) {
        cube.fillAll();
    }
    // --- Pattern Storage ---
    else if (strcmp(cmd, "savePattern") == 0) {
        anim.savePattern(doc["slot"] | 0);
    }
    else if (strcmp(cmd, "loadPattern") == 0) {
        anim.loadPattern(doc["slot"] | 0);
    }
    // --- Buffer Control ---
    else if (strcmp(cmd, "setBuffer") == 0) {
        JsonArray layers = doc["layers"];
        if (layers.size() == 4) {
            uint16_t buf[4];
            for (int i = 0; i < 4; i++) {
                buf[i] = layers[i] | 0;
            }
            cube.setBuffer(buf);
        }
    }
    // --- State Queries ---
    else if (strcmp(cmd, "getState") == 0) {
        sendStatus(clientNum);
        sendCubeState(clientNum);
    }
    else if (strcmp(cmd, "getAnims") == 0) {
        sendAnimationList(clientNum);
    }
    // --- Music Control (v2) ---
    else if (strcmp(cmd, "music_data") == 0) {
        float tempo = doc["tempo"] | 120.0f;
        float energy = doc["energy"] | 0.5f;
        float danceability = doc["danceability"] | 0.5f;
        float loudness = doc["loudness"] | -10.0f;
        float valence = doc["valence"] | 0.5f;
        float progress = doc["progress"] | 0.0f;
        bool beat = doc["beat"] | false;
        music.setMusicData(tempo, energy, danceability, loudness, valence, progress, beat);
        
        // Spectral bands
        JsonArray bands = doc["spectral"];
        if (bands && bands.size() >= 4) {
            music.setSpectralData(bands[0], bands[1], bands[2], bands[3]);
        }
    }
    else if (strcmp(cmd, "setMode") == 0) {
        const char* modeName = doc["value"];
        if (modeName) {
            if (strcmp(modeName, "spotify") == 0) music.setMode(MODE_SPOTIFY);
            else if (strcmp(modeName, "microphone") == 0) music.setMode(MODE_MICROPHONE);
            else if (strcmp(modeName, "hybrid") == 0) music.setMode(MODE_HYBRID);
            else if (strcmp(modeName, "ai") == 0) music.setMode(MODE_AI);
        }
    }
    else if (strcmp(cmd, "aiParams") == 0) {
        if (doc.containsKey("intensity")) music.setAIIntensity(doc["intensity"] | 128);
        if (doc.containsKey("randomness")) music.setAIRandomness(doc["randomness"] | 128);
        if (doc.containsKey("complexity")) music.setAIComplexity(doc["complexity"] | 128);
    }
    else if (strcmp(cmd, "aiSelect") == 0) {
        // Trigger AI auto-selection of animation
        int suggested = music.suggestAnimation();
        anim.setAnimation((AnimationType)suggested);
    }
    
    // Broadcast updated status to all clients
    broadcastStatus();
}

// --- Send Status to Specific Client ---

void WebServerManager::sendStatus(uint8_t clientNum) {
    StaticJsonDocument<512> doc;
    doc["type"] = "status";
    doc["animation"] = ANIM_NAMES[anim.getAnimation()];
    doc["animIndex"] = anim.getAnimation();
    doc["playing"] = anim.isPlaying();
    doc["autoplay"] = anim.isAutoplay();
    doc["randomMode"] = anim.isRandomMode();
    doc["speed"] = anim.getSpeed();
    doc["brightness"] = anim.getBrightness();
    doc["direction"] = anim.getDirection();
    doc["density"] = anim.getDensity();
    doc["fps"] = cube.getFPS();
    doc["heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["patterns"] = anim.getPatternCount();
    doc["ledColor"] = anim.getLedColor();
    
    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(clientNum, response);
}

// --- Send Cube State ---

void WebServerManager::sendCubeState(uint8_t clientNum) {
    StaticJsonDocument<256> doc;
    doc["type"] = "cubeState";
    
    JsonArray layers = doc.createNestedArray("layers");
    uint16_t buf[CUBE_Z];
    cube.getBuffer(buf);
    for (int i = 0; i < CUBE_Z; i++) {
        layers.add(buf[i]);
    }
    
    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(clientNum, response);
}

// --- Send Animation List ---

void WebServerManager::sendAnimationList(uint8_t clientNum) {
    StaticJsonDocument<768> doc;
    doc["type"] = "animList";
    
    JsonArray anims = doc.createNestedArray("anims");
    for (int i = 0; i < ANIM_COUNT; i++) {
        anims.add(ANIM_NAMES[i]);
    }
    
    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(clientNum, response);
}

// --- Broadcast to All Clients ---

void WebServerManager::broadcastCubeState() {
    StaticJsonDocument<256> doc;
    doc["type"] = "cubeState";
    
    JsonArray layers = doc.createNestedArray("layers");
    uint16_t buf[CUBE_Z];
    cube.getBuffer(buf);
    for (int i = 0; i < CUBE_Z; i++) {
        layers.add(buf[i]);
    }
    
    String response;
    serializeJson(doc, response);
    webSocket.broadcastTXT(response);
}

void WebServerManager::broadcastStatus() {
    StaticJsonDocument<512> doc;
    doc["type"] = "status";
    doc["animation"] = ANIM_NAMES[anim.getAnimation()];
    doc["animIndex"] = anim.getAnimation();
    doc["playing"] = anim.isPlaying();
    doc["autoplay"] = anim.isAutoplay();
    doc["randomMode"] = anim.isRandomMode();
    doc["speed"] = anim.getSpeed();
    doc["brightness"] = anim.getBrightness();
    doc["direction"] = anim.getDirection();
    doc["density"] = anim.getDensity();
    doc["fps"] = cube.getFPS();
    doc["heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["patterns"] = anim.getPatternCount();
    doc["ledColor"] = anim.getLedColor();
    
    String response;
    serializeJson(doc, response);
    webSocket.broadcastTXT(response);
}
