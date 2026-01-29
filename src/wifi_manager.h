#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiSetup {
public:
    WiFiSetup();

    // Initialize and attempt connection
    // Returns true if connected to WiFi
    bool begin(const char* apName, unsigned int timeoutSeconds);

    // Check if currently connected
    bool isConnected();

    // Get local IP address
    IPAddress getIP();

    // Get connection status string
    String getStatusString();

private:
    bool _connected;
};

#endif // WIFI_MANAGER_H
