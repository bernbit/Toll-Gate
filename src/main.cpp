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

// Fingerprint Sensor (Hardware Serial 2)
#define FP_RX 25
#define FP_TX 26
HardwareSerial fingerprintSerial(2);

// RFID Serial (Hardware Serial 1)
#define RFID_RX 16
#define RFID_TX 17
HardwareSerial RFID(1);

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

// RFID Configuration
const unsigned long DUPLICATE_WINDOW = 5000;
const int EXPECTED_BYTES = 4;
const int MIN_CONSECUTIVE_READS = 3;

struct ReadBuffer
{
  String tagID;
  int count;
  unsigned long firstSeen;
};

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

ReadBuffer currentRead = {"", 0, 0};
String lastProcessedTag = "";
unsigned long lastProcessedTime = 0;
int vehicleCount = 0;
unsigned long sessionStart = 0;
int totalReads = 0;

void setupServerRoutes();
String getFingerprintList();
void handleEnrollmentProcess();
void listLittleFSFiles();

bool isValidTag(String tagID);
void logVehiclePass(String tagID, byte *frame);
String getRFIDList();
void handleRFIDReading();

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

  // Initialize RFID
  RFID.begin(9600, SERIAL_8N1, RFID_RX, RFID_TX);
  sessionStart = millis();
  Serial.println("✓ RFID scanner initialized");

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

  // Handle RFID reading
  handleRFIDReading();

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

  // Get RFID vehicle log
  server.on("/rfid/list", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = getRFIDList();
    request->send(200, "application/json", json); });

  // Get vehicle count
  server.on("/rfid/count", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "{\"count\":" + String(vehicleCount) + "}";
    request->send(200, "application/json", json); });

  // Save vehicle data - UPDATED VERSION
  // Replace the /vehicle/save endpoint in your Arduino code with this fixed version:

  server.on("/vehicle/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
      Serial.println("JSON deserialization error!");
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }

    String rfid = doc["rfid"].as<String>();
    const char* plateNo = doc["plateNo"];
    const char* type = doc["type"];
    const char* owner = doc["owner"];
    const char* role = doc["role"];
    const char* year = doc["year"];
    const char* section = doc["section"];
    const char* course = doc["course"];

    Serial.println("\n--- Saving Vehicle ---");
    Serial.println("RFID: " + rfid);
    Serial.println("Plate: " + String(plateNo));
    Serial.println("Owner: " + String(owner));

    // Save vehicle data
  String key = "v_" + rfid;
    String value = String(plateNo) + "|" + String(type) + "|" + String(owner) + "|" + 
                   String(role) + "|" + String(year) + "|" + String(section) + "|" + String(course);
    
    bool saveSuccess = preferences.putString(key.c_str(), value);
    
    if (!saveSuccess) {
      Serial.println("✗ Failed to save vehicle data");
      request->send(500, "text/plain", "Failed to save vehicle data");
      return;
    }

    // Update vehicle list
    String vehicleListKey = "vehicle_list";
    String vehicleList = preferences.getString(vehicleListKey.c_str(), "");
    
    Serial.println("Current vehicle list: " + vehicleList);
    
    // Check if RFID already exists in list
    bool alreadyExists = false;
    if (vehicleList.length() > 0) {
      int startIdx = 0;
      for (int i = 0; i <= vehicleList.length(); i++) {
        if (i == vehicleList.length() || vehicleList[i] == ',') {
          String existingRfid = vehicleList.substring(startIdx, i);
          existingRfid.trim();
          if (existingRfid == rfid) {
            alreadyExists = true;
            break;
          }
          startIdx = i + 1;
        }
      }
    }
    
    // Add to list if not exists
    if (!alreadyExists) {
      if (vehicleList.length() > 0) {
        vehicleList += ",";
      }
      vehicleList += rfid;
      bool listSaveSuccess = preferences.putString(vehicleListKey.c_str(), vehicleList);
      
      if (!listSaveSuccess) {
        Serial.println("✗ Failed to update vehicle list");
        request->send(500, "text/plain", "Failed to update vehicle list");
        return;
      }
      
      Serial.println("Updated vehicle list: " + vehicleList);
    } else {
      Serial.println("RFID already in list (updating existing entry)");
    }

    Serial.println("✓ Vehicle saved successfully: " + rfid);
    
    // Return success response
    StaticJsonDocument<200> response;
    response["success"] = true;
    response["rfid"] = rfid;
    response["message"] = "Vehicle saved successfully";
    
    String output;
    serializeJson(response, output);
    request->send(200, "application/json", output); });

  // Get all vehicles - OPTIMIZED VERSION
  server.on("/vehicle/list", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  StaticJsonDocument<8192> doc;
  JsonArray array = doc.to<JsonArray>();

  String vehicleListKey = "vehicle_list";
  String vehicleList = preferences.getString(vehicleListKey.c_str(), "");
  
  if (vehicleList.length() > 0) {
    int startIdx = 0;
    for (int i = 0; i <= vehicleList.length(); i++) {
      if (i == vehicleList.length() || vehicleList[i] == ',') {
        String rfid = vehicleList.substring(startIdx, i);
        rfid.trim();
        
        if (rfid.length() > 0) {
          String key = "v_" + rfid;  // ✓ FIXED
          String value = preferences.getString(key.c_str(), "");
          
          if (value.length() > 0) {
            int idx = 0;
            String parts[7];
            int lastIdx = 0;
            
            for (int j = 0; j <= value.length(); j++) {
              if (j == value.length() || value[j] == '|') {
                parts[idx++] = value.substring(lastIdx, j);
                lastIdx = j + 1;
                if (idx >= 7) break;
              }
            }
            
            JsonObject obj = array.createNestedObject();
            obj["rfid"] = rfid;
            obj["plateNo"] = parts[0];
            obj["type"] = parts[1];
            obj["owner"] = parts[2];
            obj["role"] = parts[3];
            obj["year"] = parts[4];
            obj["section"] = parts[5];
            obj["course"] = parts[6];
          }
        }
        
        startIdx = i + 1;
      }
    }
  }
  
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output); });

  // Delete vehicle - UPDATED VERSION
  server.on("/vehicle/delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  if (!request->hasParam("rfid")) {
    request->send(400, "text/plain", "Missing 'rfid' parameter");
    return;
  }

  String rfid = request->getParam("rfid")->value();
  String key = "v_" + rfid;  // ✓ FIXED
  preferences.remove(key.c_str());
  
  // Remove from vehicle list
  String vehicleListKey = "vehicle_list";
  String vehicleList = preferences.getString(vehicleListKey.c_str(), "");
  
  String newList = "";
  int startIdx = 0;
  for (int i = 0; i <= vehicleList.length(); i++) {
    if (i == vehicleList.length() || vehicleList[i] == ',') {
      String currentRfid = vehicleList.substring(startIdx, i);
      currentRfid.trim();
      
      if (currentRfid != rfid && currentRfid.length() > 0) {
        if (newList.length() > 0) newList += ",";
        newList += currentRfid;
      }
      
      startIdx = i + 1;
    }
  }
  
  preferences.putString(vehicleListKey.c_str(), newList);
  
  Serial.println("✓ Vehicle " + rfid + " deleted");
  request->send(200, "text/plain", "Vehicle deleted"); });

  // Delete all vehicles - UPDATED VERSION
  server.on("/vehicle/deleteall", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Serial.println("\n--- Deleting ALL vehicles ---");
  
  String vehicleListKey = "vehicle_list";
  String vehicleList = preferences.getString(vehicleListKey.c_str(), "");
  
  int deletedCount = 0;
  
  int startIdx = 0;
  for (int i = 0; i <= vehicleList.length(); i++) {
    if (i == vehicleList.length() || vehicleList[i] == ',') {
      String rfid = vehicleList.substring(startIdx, i);
      rfid.trim();
      
      if (rfid.length() > 0) {
        String key = "v_" + rfid;  // ✓ FIXED
        preferences.remove(key.c_str());
        deletedCount++;
        Serial.println("  ✓ Deleted " + rfid);
      }
      
      startIdx = i + 1;
    }
  }
  
  preferences.remove(vehicleListKey.c_str());
  
  Serial.println("✓ Deleted " + String(deletedCount) + " vehicles");
  request->send(200, "text/plain", "All vehicles deleted (" + String(deletedCount) + " removed)"); });

  // Start RFID scan
  server.on("/rfid/startscan", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    events.send("Waiting for RFID tag...", "rfid_status", millis());
    request->send(200, "text/plain", "RFID scan started"); });

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

// ============================================================================
// RFID FUNCTIONS
// ============================================================================

// ============================================================================
// RFID FUNCTIONS (UPDATED)
// ============================================================================

// ===============================
// HANDLE RFID READING
// ===============================
void handleRFIDReading()
{
  static byte buffer[EXPECTED_BYTES];
  static int bufferIndex = 0;
  static unsigned long lastByteTime = 0;

  while (RFID.available())
  {
    if (bufferIndex < EXPECTED_BYTES)
    {
      buffer[bufferIndex++] = RFID.read();
      lastByteTime = millis();
    }
    else
    {
      RFID.read(); // discard extra bytes
    }
  }

  if (bufferIndex == EXPECTED_BYTES && (millis() - lastByteTime > 100))
  {
    totalReads++;

    // Construct 6–8 character tag ID from raw bytes
    String tagID = "";
    for (int i = 0; i < EXPECTED_BYTES; i++)
    {
      if (buffer[i] < 16)
        tagID += "0";
      tagID += String(buffer[i], HEX);
    }
    tagID.toUpperCase();

    if (isValidTag(tagID))
    {
      if (tagID == currentRead.tagID)
      {
        currentRead.count++;
      }
      else
      {
        currentRead.tagID = tagID;
        currentRead.count = 1;
        currentRead.firstSeen = millis();
      }

      if (currentRead.count >= MIN_CONSECUTIVE_READS)
      {
        unsigned long now = millis();
        if (tagID != lastProcessedTag || (now - lastProcessedTime > DUPLICATE_WINDOW))
        {
          vehicleCount++;
          logVehiclePass(tagID, buffer); // use updated function
          lastProcessedTag = tagID;
          lastProcessedTime = now;

          // Send SSE notification
          events.send(("Vehicle detected: " + tagID).c_str(), "rfid", millis());
        }
        currentRead.count = 0;
      }
    }

    bufferIndex = 0;
  }

  if (bufferIndex > 0 && (millis() - lastByteTime > 500))
  {
    bufferIndex = 0;
  }

  if (currentRead.tagID.length() > 0 && (millis() - currentRead.firstSeen > 2000))
  {
    currentRead.tagID = "";
    currentRead.count = 0;
  }
}

bool isValidTag(String tagID)
{
  if (tagID == "0000" || tagID == "FFFF")
    return false;
  if (tagID == "0001" || tagID.length() != 8)
    return false; // now expecting 8 chars
  return true;
}

// ===============================
// LOG VEHICLE PASS
// ===============================
void logVehiclePass(String tagID, byte *frame)
{
  unsigned long timestamp = millis() - sessionStart;
  int seconds = timestamp / 1000;

  Serial.println("\n--- VEHICLE DETECTED ---");
  Serial.println("Vehicle #" + String(vehicleCount));
  Serial.println("Tag ID: " + tagID);
  Serial.println("Time: " + String(seconds) + "s");

  // Check if this vehicle is registered
  String vehicleKey = "v_" + tagID;
  String vehicleData = preferences.getString(vehicleKey.c_str(), "");

  if (vehicleData.length() > 0)
  {
    // Parse vehicle data
    int idx = 0;
    String parts[7];
    int lastIdx = 0;

    for (int j = 0; j <= vehicleData.length(); j++)
    {
      if (j == vehicleData.length() || vehicleData[j] == '|')
      {
        parts[idx++] = vehicleData.substring(lastIdx, j);
        lastIdx = j + 1;
        if (idx >= 7)
          break;
      }
    }

    Serial.println("  Registered Vehicle:");
    Serial.println("  Plate: " + parts[0]);
    Serial.println("  Owner: " + parts[2]);
    Serial.println("  Role: " + parts[3]);
  }
  else
  {
    Serial.println("  ⚠ Unregistered vehicle!");
  }

  Serial.println("------------------------\n");

  // Log the pass event (separate from vehicle registration)
  // Use a different key structure for pass logs
  String passKey = "pass_" + String(vehicleCount);
  String passValue = tagID + "|" + String(timestamp);
  preferences.putString(passKey.c_str(), passValue);

  // Update pass log list (separate from vehicle list)
  String passListKey = "pass_list";
  String passList = preferences.getString(passListKey.c_str(), "");

  if (passList.length() > 0)
    passList += ",";
  passList += String(vehicleCount);

  preferences.putString(passListKey.c_str(), passList);
}
// ===============================
// GET RFID LIST
// ===============================
String getRFIDList()
{
  StaticJsonDocument<4096> doc;
  JsonArray array = doc.to<JsonArray>();

  String passListKey = "pass_list";
  String passList = preferences.getString(passListKey.c_str(), "");

  int startIdx = 0;
  for (int i = 0; i <= passList.length(); i++)
  {
    if (i == passList.length() || passList[i] == ',')
    {
      String countStr = passList.substring(startIdx, i);
      countStr.trim();
      if (countStr.length() > 0)
      {
        int vNum = countStr.toInt();
        String key = "pass_" + String(vNum);
        String value = preferences.getString(key.c_str(), "");
        if (value.length() > 0)
        {
          int sep = value.indexOf('|');
          String tagID = value.substring(0, sep);
          String timestamp = value.substring(sep + 1);

          JsonObject obj = array.createNestedObject();
          obj["id"] = vNum;
          obj["tagID"] = tagID;
          obj["timestamp"] = timestamp;

          // Add vehicle info if registered
          String vehicleKey = "v_" + tagID;
          String vehicleData = preferences.getString(vehicleKey.c_str(), "");

          if (vehicleData.length() > 0)
          {
            int idx = 0;
            String parts[7];
            int lastIdx = 0;

            for (int j = 0; j <= vehicleData.length(); j++)
            {
              if (j == vehicleData.length() || vehicleData[j] == '|')
              {
                parts[idx++] = vehicleData.substring(lastIdx, j);
                lastIdx = j + 1;
                if (idx >= 7)
                  break;
              }
            }

            obj["plateNo"] = parts[0];
            obj["owner"] = parts[2];
            obj["registered"] = true;
          }
          else
          {
            obj["plateNo"] = "Unregistered";
            obj["owner"] = "Unknown";
            obj["registered"] = false;
          }
        }
      }
      startIdx = i + 1;
    }
  }

  String output;
  serializeJson(doc, output);
  return output;
}