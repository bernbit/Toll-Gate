#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

// AP Configuration
const char *ssid = "MINSU TollGate";
const char *password = "minsu_tollgate_0001"; // At least 8 characters

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const int ledPin = 2; // Built-in LED

void setup()
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);

    // Start LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed!");
        return;
    }
    Serial.println("LittleFS mounted successfully!");

    // Start ESP32 in AP mode
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.softAPIP());

    // Serve files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Start server
    server.begin();
    Serial.println("Web server started!");
}

void loop()
{
}
