#include <HardwareSerial.h>

HardwareSerial RFID(2);

// Configuration
const unsigned long DUPLICATE_WINDOW = 5000; // 5 seconds between same vehicle
const int EXPECTED_BYTES = 4;
const int MIN_CONSECUTIVE_READS = 3;

// Vehicle tracking
struct ReadBuffer
{
    String tagID;
    int count;
    unsigned long firstSeen;
};

ReadBuffer currentRead = {"", 0, 0};
String lastProcessedTag = "";
unsigned long lastProcessedTime = 0;
int vehicleCount = 0;

// Statistics
unsigned long sessionStart = 0;
int totalReads = 0;

void setup()
{
    Serial.begin(115200);
    RFID.begin(9600, SERIAL_8N1, 16, 17);

    sessionStart = millis();

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║    🚗 TOLLGATE RFID SYSTEM v1.0 🚗     ║");
    Serial.println("║    Ready for Operation                 ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    Serial.println("System initialized. Monitoring traffic...\n");
}

void loop()
{
    static byte buffer[EXPECTED_BYTES];
    static int bufferIndex = 0;
    static unsigned long lastByteTime = 0;

    // Read incoming data
    while (RFID.available())
    {
        if (bufferIndex < EXPECTED_BYTES)
        {
            buffer[bufferIndex++] = RFID.read();
            lastByteTime = millis();
        }
        else
        {
            RFID.read(); // Discard overflow
        }
    }

    // Process complete packet
    if (bufferIndex == EXPECTED_BYTES && (millis() - lastByteTime > 100))
    {
        totalReads++;

        // Extract tag ID (bytes 1-2)
        String tagID = "";
        if (buffer[1] < 16)
            tagID += "0";
        tagID += String(buffer[1], HEX);
        if (buffer[2] < 16)
            tagID += "0";
        tagID += String(buffer[2], HEX);
        tagID.toUpperCase();

        // Validate tag ID
        if (isValidTag(tagID))
        {

            // Count consecutive identical reads
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

            // Process when verified (3+ consecutive reads)
            if (currentRead.count >= MIN_CONSECUTIVE_READS)
            {
                unsigned long now = millis();

                // Check if this is a new vehicle (not duplicate)
                if (tagID != lastProcessedTag || (now - lastProcessedTime > DUPLICATE_WINDOW))
                {
                    vehicleCount++;
                    logVehiclePass(tagID, buffer);
                    lastProcessedTag = tagID;
                    lastProcessedTime = now;
                }

                currentRead.count = 0; // Reset to prevent re-logging
            }
        }

        bufferIndex = 0;
    }

    // Timeout reset
    if (bufferIndex > 0 && (millis() - lastByteTime > 500))
    {
        bufferIndex = 0;
    }

    // Clear stale reads
    if (currentRead.tagID.length() > 0 && (millis() - currentRead.firstSeen > 2000))
    {
        currentRead.tagID = "";
        currentRead.count = 0;
    }

    // Display statistics every 60 seconds
    displayStats();
}

bool isValidTag(String tagID)
{
    // Filter invalid patterns
    if (tagID == "0000" || tagID == "FFFF")
        return false;
    if (tagID == "0001" || tagID.length() != 4)
        return false;

    // Optional: Add your registered tag validation here
    // if (!isRegisteredTag(tagID)) return false;

    return true;
}

void logVehiclePass(String tagID, byte *frame)
{
    // Convert to decimal
    byte b1 = 0, b2 = 0;
    sscanf(tagID.c_str(), "%2hhx%2hhx", &b1, &b2);
    unsigned long tagDecimal = (b1 << 8) | b2;

    // Get timestamp
    unsigned long timestamp = millis() - sessionStart;
    int seconds = timestamp / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;

    // Display vehicle info
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║        🚗 VEHICLE PASSED 🚗            ║");
    Serial.println("╠════════════════════════════════════════╣");

    Serial.print("║ Vehicle #:     ");
    Serial.print(vehicleCount);
    if (vehicleCount < 10)
        Serial.print("   ");
    else if (vehicleCount < 100)
        Serial.print("  ");
    else if (vehicleCount < 1000)
        Serial.print(" ");
    Serial.println("                     ║");

    Serial.print("║ Tag ID (HEX):  ");
    Serial.print(tagID);
    Serial.println("                        ║");

    Serial.print("║ Tag ID (DEC):  ");
    Serial.print(tagDecimal);
    if (tagDecimal < 10)
        Serial.print("    ");
    else if (tagDecimal < 100)
        Serial.print("   ");
    else if (tagDecimal < 1000)
        Serial.print("  ");
    else if (tagDecimal < 10000)
        Serial.print(" ");
    Serial.println("                     ║");

    Serial.print("║ Time:          ");
    if (hours > 0)
    {
        Serial.print(hours);
        Serial.print("h ");
    }
    if (minutes % 60 > 0)
    {
        Serial.print(minutes % 60);
        Serial.print("m ");
    }
    Serial.print(seconds % 60);
    Serial.print("s");
    Serial.println("                       ║");

    Serial.print("║ Raw Data:      ");
    for (int i = 0; i < EXPECTED_BYTES; i++)
    {
        if (frame[i] < 16)
            Serial.print("0");
        Serial.print(frame[i], HEX);
        Serial.print(" ");
    }
    Serial.println("            ║");

    Serial.println("╚════════════════════════════════════════╝\n");

    void displayStats()
    {
        static unsigned long lastStatsTime = 0;

        if (millis() - lastStatsTime > 60000)
        { // Every 60 seconds
            unsigned long uptime = (millis() - sessionStart) / 1000;

            Serial.println("╔════════════════════════════════════════╗");
            Serial.println("║         📊 SYSTEM STATISTICS 📊        ║");
            Serial.println("╠════════════════════════════════════════╣");
            Serial.print("║ Total Vehicles:   ");
            Serial.print(vehicleCount);
            Serial.println("                     ║");
            Serial.print("║ Total Reads:      ");
            Serial.print(totalReads);
            Serial.println("                     ║");
            Serial.print("║ Uptime:           ");
            Serial.print(uptime / 60);
            Serial.print(" min");
            Serial.println("                  ║");
            Serial.println("╚════════════════════════════════════════╝\n");

            lastStatsTime = millis();
        }
    }