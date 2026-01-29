#include "wifi_manager.h"
#include "config.h"
#include <WiFiManager.h>

WiFiSetup::WiFiSetup() : _connected(false) {
}

bool WiFiSetup::begin(const char* apName, unsigned int timeoutSeconds) {
    WiFiManager wm;

    // Set timeout for configuration portal
    wm.setConfigPortalTimeout(timeoutSeconds);

    // Set dark theme for portal
    wm.setClass("invert");

    // Automatically connect using saved credentials,
    // or start configuration portal if none saved or connection fails
    DEBUG_SERIAL.println("WiFiManager: Attempting connection...");

    _connected = wm.autoConnect(apName);

    if (_connected) {
        DEBUG_SERIAL.println("WiFiManager: Connected to WiFi");
        DEBUG_SERIAL.print("IP Address: ");
        DEBUG_SERIAL.println(WiFi.localIP());
    } else {
        DEBUG_SERIAL.println("WiFiManager: Failed to connect");
    }

    return _connected;
}

bool WiFiSetup::isConnected() {
    _connected = (WiFi.status() == WL_CONNECTED);
    return _connected;
}

IPAddress WiFiSetup::getIP() {
    return WiFi.localIP();
}

String WiFiSetup::getStatusString() {
    if (isConnected()) {
        return "Connected: " + WiFi.localIP().toString();
    }
    return "Disconnected";
}
