#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// Access Point Credentials
const char *AP_SSID = "TOLL_GATE_SYSTEM";
const char *AP_PASSWORD = "12345678"; // At least 8 characters

// Fingerprint Sensor Serial (Hardware Serial 2)
#define FP_RX 25 // ESP32 RX -> Sensor TX (Yellow wire)
#define FP_TX 26 // ESP32 TX -> Sensor RX (White wire)
HardwareSerial fingerprintSerial(2);

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

AsyncWebServer server(80);
AsyncEventSource events("/events");
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerprintSerial);
Preferences preferences;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

bool enrollmentInProgress = false;
int currentEnrollID = -1;
unsigned long lastSSECheck = 0;

void setupServerRoutes();
String getFingerprintList();
void handleEnrollmentProcess();
void listLittleFSFiles();
// ============================================================================
// SETUP
// ============================================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=================================");
  Serial.println("ESP32 Fingerprint Toll Gate System");
  Serial.println("=================================\n");

  // Initialize LittleFS
  if (!LittleFS.begin(true))
  {
    Serial.println("ERROR: LittleFS Mount Failed!");
    Serial.println("Please upload filesystem image using:");
    Serial.println("Arduino IDE: Tools -> ESP32 Sketch Data Upload");
    Serial.println("PlatformIO: pio run --target uploadfs");
    return;
  }
  Serial.println("✓ LittleFS mounted successfully");

  // List files in LittleFS
  listLittleFSFiles();

  // Initialize Fingerprint Sensor
  fingerprintSerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  delay(100);

  if (finger.verifyPassword())
  {
    Serial.println("✓ Fingerprint sensor connected!");
    Serial.print("  Sensor capacity: ");
    Serial.println(finger.capacity);
    Serial.print("  Currently enrolled: ");
    Serial.println(finger.templateCount);
  }
  else
  {
    Serial.println("ERROR: Fingerprint sensor not found!");
    Serial.println("Check wiring:");
    Serial.println("  - Red wire -> 3.3V");
    Serial.println("  - Black wire -> GND");
    Serial.println("  - Yellow wire (TX) -> GPIO 16 (RX)");
    Serial.println("  - White wire (RX) -> GPIO 17 (TX)");
  }

  // Initialize Preferences (for metadata storage)
  preferences.begin("fingerprints", false);
  Serial.println("✓ Preferences initialized");

  // Setup WiFi Access Point
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n✓ Access Point Started");
  Serial.print("  SSID: ");
  Serial.println(AP_SSID);
  Serial.print("  Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("  IP Address: http://");
  Serial.println(IP);
  Serial.println("\nConnect to this WiFi and open the IP in browser\n");

  // Setup Web Server Routes
  setupServerRoutes();

  // Start SSE
  server.addHandler(&events);

  // Start Server
  server.begin();
  Serial.println("✓ Web server started\n");
  Serial.println("=================================");
  Serial.println("System Ready!");
  Serial.println("=================================\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop()
{
  // Handle enrollment process if active
  if (enrollmentInProgress)
  {
    handleEnrollmentProcess();
  }

  // Keep SSE connections alive
  if (millis() - lastSSECheck > 1000)
  {
    events.send("ping", NULL, millis());
    lastSSECheck = millis();
  }

  delay(10);
}

// ============================================================================
// WEB SERVER ROUTES
// ============================================================================

void setupServerRoutes()
{

  // Serve main index page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      request->send(404, "text/plain", "index.html not found in LittleFS");
    } });

  // Serve fingerprint page
  server.on("/fingerprint.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/fingerprint.html")) {
      request->send(LittleFS, "/fingerprint.html", "text/html");
    } else {
      request->send(404, "text/plain", "fingerprint.html not found");
    } });

  // Serve vehicle page
  server.on("/vehicle.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/vehicle.html")) {
      request->send(LittleFS, "/vehicle.html", "text/html");
    } else {
      request->send(404, "text/plain", "vehicle.html not found");
    } });

  // Serve settings page
  server.on("/setting.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/setting.html")) {
      request->send(LittleFS, "/setting.html", "text/html");
    } else {
      request->send(404, "text/plain", "setting.html not found");
    } });

  // Serve favicon
  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/favicon.svg")) {
      request->send(LittleFS, "/favicon.svg", "image/svg+xml");
    } else {
      request->send(404, "text/plain", "favicon.svg not found");
    } });

  // Serve CSS
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/style.css")) {
      request->send(LittleFS, "/style.css", "text/css");
    } else {
      request->send(404, "text/plain", "style.css not found");
    } });

  // Serve JavaScript
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (LittleFS.exists("/script.js")) {
      request->send(LittleFS, "/script.js", "application/javascript");
    } else {
      request->send(404, "text/plain", "script.js not found");
    } });

  // Get list of enrolled fingerprints
  server.on("/fp/list", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = getFingerprintList();
    request->send(200, "application/json", json); });

  // Start fingerprint enrollment
  server.on("/fp/enroll", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing 'id' parameter");
      return;
    }

    int id = request->getParam("id")->value().toInt();
    
    if (id < 1 || id > 127) {
      request->send(400, "text/plain", "ID must be between 1 and 127");
      return;
    }

    if (enrollmentInProgress) {
      request->send(409, "text/plain", "Enrollment already in progress");
      return;
    }

    currentEnrollID = id;
    enrollmentInProgress = true;
    
    Serial.println("\n--- Starting Enrollment for ID " + String(id) + " ---");
    events.send("Starting enrollment process...", "status", millis());
    
    request->send(200, "text/plain", "Enrollment started for ID " + String(id)); });

  // Save fingerprint metadata
  server.on("/fp/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, data);
      
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      int id = doc["id"];
      const char* owner = doc["owner"];
      const char* role = doc["role"];

      // Save to Preferences
      String key = "fp_" + String(id);
      String value = String(owner) + "|" + String(role);
      preferences.putString(key.c_str(), value);

      Serial.println("✓ Metadata saved:");
      Serial.println("  ID: " + String(id));
      Serial.println("  Owner: " + String(owner));
      Serial.println("  Role: " + String(role));

      request->send(200, "application/json", "{\"success\":true}"); });

  // Delete fingerprint
  server.on("/fp/delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing 'id' parameter");
      return;
    }

    int id = request->getParam("id")->value().toInt();
    
    // Delete from sensor
    uint8_t result = finger.deleteModel(id);
    
    if (result == FINGERPRINT_OK) {
      // Delete metadata
      String key = "fp_" + String(id);
      preferences.remove(key.c_str());
      
      Serial.println("✓ Fingerprint " + String(id) + " deleted");
      request->send(200, "text/plain", "Fingerprint deleted");
    } else {
      request->send(500, "text/plain", "Failed to delete fingerprint");
    } });

  // Delete ALL fingerprints
  server.on("/fp/deleteall", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Serial.println("\n--- Deleting ALL fingerprints ---");
  
  int deletedCount = 0;
  int failedCount = 0;
  
  // Loop through all possible IDs (1-127)
  for (int id = 1; id <= 127; id++) {
    // Check if template exists
    uint8_t result = finger.loadModel(id);
    
    if (result == FINGERPRINT_OK) {
      // Template exists, delete it
      result = finger.deleteModel(id);
      
      if (result == FINGERPRINT_OK) {
        // Delete metadata
        String key = "fp_" + String(id);
        preferences.remove(key.c_str());
        deletedCount++;
        Serial.println("  ✓ Deleted ID " + String(id));
      } else {
        failedCount++;
        Serial.println("  ✗ Failed to delete ID " + String(id));
      }
    }
  }
  
  Serial.println("--- Delete All Complete ---");
  Serial.println("  Deleted: " + String(deletedCount));
  Serial.println("  Failed: " + String(failedCount));
  Serial.println("---------------------------\n");
  
  if (failedCount == 0) {
    request->send(200, "text/plain", "All fingerprints deleted (" + String(deletedCount) + " removed)");
  } else {
    request->send(500, "text/plain", "Partially completed. " + String(deletedCount) + " deleted, " + String(failedCount) + " failed");
  } });

  // 404 handler
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Not Found"); });
}

// ============================================================================
// FINGERPRINT FUNCTIONS
// ============================================================================

String getFingerprintList()
{
  StaticJsonDocument<4096> doc;
  JsonArray array = doc.to<JsonArray>();

  // Get all enrolled templates from sensor
  finger.getTemplateCount();

  Serial.println("\n--- Scanning for enrolled fingerprints ---");

  // Check each slot (1-127)
  for (int id = 1; id <= 127; id++)
  {
    // Check if template exists in this slot
    uint8_t result = finger.loadModel(id);

    if (result == FINGERPRINT_OK)
    {
      // Template exists, get metadata
      String key = "fp_" + String(id);
      String value = preferences.getString(key.c_str(), "Unknown|Unknown");

      // Parse owner and role
      int separatorIndex = value.indexOf('|');
      String owner = value.substring(0, separatorIndex);
      String role = value.substring(separatorIndex + 1);

      // Add to JSON array
      JsonObject obj = array.createNestedObject();
      obj["id"] = id;
      obj["owner"] = owner;
      obj["role"] = role;

      Serial.println("  Found ID " + String(id) + ": " + owner + " (" + role + ")");
    }
  }

  Serial.println("--- Scan complete ---\n");

  String output;
  serializeJson(doc, output);
  return output;
}

void handleEnrollmentProcess()
{
  static int stage = 0;
  static unsigned long stageStartTime = 0;

  // Initialize stage timing
  if (stage == 0)
  {
    stage = 1;
    stageStartTime = millis();
    events.send("Place your finger on the sensor", "prompt", millis());
    Serial.println("Stage 1: Waiting for finger...");
  }

  // Stage 1: Get first image
  if (stage == 1)
  {
    int result = finger.getImage();

    if (result == FINGERPRINT_OK)
    {
      Serial.println("✓ Image captured");
      events.send("Image captured, processing...", "status", millis());

      result = finger.image2Tz(1);
      if (result == FINGERPRINT_OK)
      {
        Serial.println("✓ Image converted");
        stage = 2;
        stageStartTime = millis();
        events.send("Remove your finger", "prompt", millis());
        Serial.println("Stage 2: Waiting for finger removal...");
      }
      else
      {
        Serial.println("✗ Image conversion failed");
        events.send("Image quality too low, please try again", "error", millis());
        enrollmentInProgress = false;
        stage = 0;
      }
    }
    else if (result == FINGERPRINT_NOFINGER)
    {
      // Still waiting for finger
      if (millis() - stageStartTime > 10000)
      {
        events.send("Timeout - please try again", "error", millis());
        enrollmentInProgress = false;
        stage = 0;
      }
    }
    else
    {
      Serial.println("✗ Error getting image: " + String(result));
      events.send("Sensor error, please try again", "error", millis());
      enrollmentInProgress = false;
      stage = 0;
    }
  }

  // Stage 2: Wait for finger removal
  else if (stage == 2)
  {
    if (finger.getImage() == FINGERPRINT_NOFINGER)
    {
      stage = 3;
      stageStartTime = millis();
      events.send("Place the SAME finger again", "prompt", millis());
      Serial.println("Stage 3: Waiting for same finger again...");
      delay(1000); // Give user time to see message
    }
  }

  // Stage 3: Get second image
  else if (stage == 3)
  {
    int result = finger.getImage();

    if (result == FINGERPRINT_OK)
    {
      Serial.println("✓ Second image captured");
      events.send("Image captured, processing...", "status", millis());

      result = finger.image2Tz(2);
      if (result == FINGERPRINT_OK)
      {
        Serial.println("✓ Second image converted");

        // Create model
        result = finger.createModel();
        if (result == FINGERPRINT_OK)
        {
          Serial.println("✓ Fingerprint model created");

          // Store model
          result = finger.storeModel(currentEnrollID);
          if (result == FINGERPRINT_OK)
          {
            Serial.println("✓ Fingerprint stored at ID " + String(currentEnrollID));
            events.send("Fingerprint enrolled successfully! ID: " + String(currentEnrollID), "done", millis());
            enrollmentInProgress = false;
            stage = 0;
          }
          else
          {
            Serial.println("✗ Failed to store model");
            events.send("Failed to save fingerprint", "error", millis());
            enrollmentInProgress = false;
            stage = 0;
          }
        }
        else if (result == FINGERPRINT_ENROLLMISMATCH)
        {
          Serial.println("✗ Fingerprints do not match");
          events.send("Fingerprints did not match, please try again", "error", millis());
          enrollmentInProgress = false;
          stage = 0;
        }
        else
        {
          Serial.println("✗ Failed to create model: " + String(result));
          events.send("Failed to process fingerprint", "error", millis());
          enrollmentInProgress = false;
          stage = 0;
        }
      }
      else
      {
        Serial.println("✗ Second image conversion failed");
        events.send("Image quality too low, please try again", "error", millis());
        enrollmentInProgress = false;
        stage = 0;
      }
    }
    else if (result == FINGERPRINT_NOFINGER)
    {
      // Still waiting
      if (millis() - stageStartTime > 10000)
      {
        events.send("Timeout - please try again", "error", millis());
        enrollmentInProgress = false;
        stage = 0;
      }
    }
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void listLittleFSFiles()
{
  Serial.println("\n--- LittleFS Files ---");
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  bool foundFiles = false;
  while (file)
  {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    foundFiles = true;
    file = root.openNextFile();
  }

  if (!foundFiles)
  {
    Serial.println("  No files found!");
    Serial.println("  Upload your HTML/CSS/JS files to LittleFS");
  }
  Serial.println("----------------------\n");
}