#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// AP Configuration
const char *ssid = "ESP32-AP";
const char *password = "12345678"; // At least 8 characters

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Variable to store LED state
bool ledState = false;
const int ledPin = 2; // Built-in LED on most ESP32 boards

void setup()
{
    Serial.begin(115200);

    // Initialize LED pin
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // Configure Access Point
    Serial.println("Setting up Access Point...");
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; margin-top: 50px; background: #f0f0f0; }";
    html += "h1 { color: #333; }";
    html += ".button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px;";
    html += "text-decoration: none; font-size: 16px; margin: 10px; cursor: pointer; border-radius: 4px; }";
    html += ".button-off { background-color: #f44336; }";
    html += ".status { font-size: 24px; margin: 20px; padding: 10px; }";
    html += "</style></head><body>";
    html += "<h1>ESP32 Web Server</h1>";
    html += "<p class='status'>LED Status: <b>" + String(ledState ? "ON" : "OFF") + "</b></p>";
    html += "<p><a href='/led/on'><button class='button'>Turn ON</button></a></p>";
    html += "<p><a href='/led/off'><button class='button button-off'>Turn OFF</button></a></p>";
    html += "<p><a href='/status'><button class='button'>Get Status</button></a></p>";
    html += "</body></html>";
    
    request->send(200, "text/html", html); });

    // Route to turn LED on
    server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    ledState = true;
    digitalWrite(ledPin, HIGH);
    Serial.println("LED turned ON");
    request->redirect("/"); });

    // Route to turn LED off
    server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    ledState = false;
    digitalWrite(ledPin, LOW);
    Serial.println("LED turned OFF");
    request->redirect("/"); });

    // Route to get status as JSON
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String json = "{\"led\":";
    json += ledState ? "true" : "false";
    json += ",\"clients\":";
    json += WiFi.softAPgetStationNum();
    json += "}";
    request->send(200, "application/json", json); });

    // Route to handle form data (POST example)
    server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request)
              {
    String message = "Data received: ";
    if(request->hasParam("data", true)) {
      message += request->getParam("data", true)->value();
    }
    request->send(200, "text/plain", message); });

    // Handle not found
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });

    // Start server
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Connect to WiFi network: " + String(ssid));
    Serial.println("Then open http://" + IP.toString() + " in browser");
}

void loop()
{
    // Print number of connected clients every 10 seconds
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000)
    {
        Serial.print("Connected clients: ");
        Serial.println(WiFi.softAPgetStationNum());
        lastCheck = millis();
    }

    delay(100);
}