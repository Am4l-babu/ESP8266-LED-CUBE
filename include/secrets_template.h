/*
 * secrets_template.h - WiFi & Security Credentials TEMPLATE
 * 
 * ╔══════════════════════════════════════════════════════════╗
 * ║  SETUP: Copy this file to secrets.h and fill in your    ║
 * ║  actual WiFi credentials before building the firmware.  ║
 * ╚══════════════════════════════════════════════════════════╝
 * 
 *   cp include/secrets_template.h include/secrets.h
 * 
 * secrets.h is in .gitignore and will NOT be committed.
 */

#ifndef SECRETS_H
#define SECRETS_H

// --- WiFi Station Mode (connect to your home router) ---
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// --- WiFi AP Fallback (auto-created if STA connection fails) ---
#define AP_SSID       "LED-Cube-AP"
#define AP_PASSWORD   "ledcube123"

// --- OTA Hostname (for wireless firmware updates) ---
#define OTA_HOSTNAME  "led-cube"

#endif // SECRETS_H
