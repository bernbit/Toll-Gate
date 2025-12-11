/* ESP32 Fingerprint Web Enrollment - AP Mode
   - Requires: AsyncTCP, ESPAsyncWebServer, Adafruit_Fingerprint
   - Creates WiFi Access Point
   - Serves web UI, /fp/list, /fp/enroll?id=NN, and SSE at /events
*/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Fingerprint.h>

// ---------------- CONFIG ----------------
const char *ap_ssid = "ESP32-Fingerprint"; // AP network name
const char *ap_password = "12345678";      // AP password (min 8 chars, leave empty for open network)

IPAddress local_IP(192, 168, 4, 1); // AP IP address
IPAddress gateway(192, 168, 4, 1);  // Gateway
IPAddress subnet(255, 255, 255, 0); // Subnet mask

const int FINGER_RX = 25;       // sensor TX -> ESP32 RX (GPIO)
const int FINGER_TX = 26;       // sensor RX -> ESP32 TX (GPIO)
HardwareSerial FingerSerial(2); // use UART2

AsyncWebServer server(80);
AsyncEventSource events("/events"); // SSE endpoint

Adafruit_Fingerprint finger(&FingerSerial);

// Track if an enrollment task is running
volatile bool enrollmentInProgress = false;
// optional: last result message
String lastEnrollResult = "";

// ---------- Utility: send SSE message ----------
void sendEvent(const char *msg, const char *type = "status")
{
    events.send(msg, type);
}

// ---------- Fingerprint helper: check if ID exists ----------
bool fingerprintExists(uint8_t id)
{
    // try to load model, returns FINGERPRINT_OK if model exists
    return (finger.loadModel(id) == FINGERPRINT_OK);
}

// ---------- Enrollment function run in a FreeRTOS task ----------
void enrollmentTask(void *param)
{
    int id = *((int *)param);
    free(param);

    // Make sure only one enrollment at a time
    if (enrollmentInProgress)
    {
        sendEvent("Another enrollment is already in progress", "error");
        vTaskDelete(NULL);
        return;
    }
    enrollmentInProgress = true;
    lastEnrollResult = "";

    char buf[128];

    sprintf(buf, "Starting enrollment for ID %d", id);
    sendEvent(buf, "status");

    // Step 1: wait for finger and capture first image
    int p = -1;
    sendEvent("Place finger (1/2)", "prompt");
    while (true)
    {
        p = finger.getImage();
        if (p == FINGERPRINT_OK)
        {
            sendEvent("Image taken (1/2)", "status");
            break;
        }
        else if (p == FINGERPRINT_NOFINGER)
        {
            delay(200);
            continue;
        }
        else
        {
            sendEvent("Error capturing image (1/2)", "error");
            enrollmentInProgress = false;
            vTaskDelete(NULL);
            return;
        }
    }

    // convert to template 1
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK)
    {
        sendEvent("Failed to convert image 1", "error");
        enrollmentInProgress = false;
        vTaskDelete(NULL);
        return;
    }
    sendEvent("Template 1 created. Remove finger", "status");

    // Wait for finger removal
    while (finger.getImage() != FINGERPRINT_NOFINGER)
    {
        delay(100);
    }

    // Step 2: second scan
    sendEvent("Place same finger (2/2)", "prompt");
    p = -1;
    while (true)
    {
        p = finger.getImage();
        if (p == FINGERPRINT_OK)
        {
            sendEvent("Image taken (2/2)", "status");
            break;
        }
        else if (p == FINGERPRINT_NOFINGER)
        {
            delay(200);
            continue;
        }
        else
        {
            sendEvent("Error capturing image (2/2)", "error");
            enrollmentInProgress = false;
            vTaskDelete(NULL);
            return;
        }
    }

    // convert to template 2
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK)
    {
        sendEvent("Failed to convert image 2", "error");
        enrollmentInProgress = false;
        vTaskDelete(NULL);
        return;
    }

    // create model
    p = finger.createModel();
    if (p != FINGERPRINT_OK)
    {
        if (p == FINGERPRINT_ENROLLMISMATCH)
            sendEvent("Fingerprints did not match", "error");
        else
            sendEvent("Failed to create model", "error");
        enrollmentInProgress = false;
        vTaskDelete(NULL);
        return;
    }
    sendEvent("Model created. Storing...", "status");

    // store model into sensor flash
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK)
    {
        sprintf(buf, "Stored fingerprint as ID %d", id);
        sendEvent(buf, "done");
        lastEnrollResult = String(buf);
    }
    else
    {
        if (p == FINGERPRINT_BADLOCATION)
            sendEvent("Could not store: bad location", "error");
        else if (p == FINGERPRINT_FLASHERR)
            sendEvent("Could not store: flash error", "error");
        else
            sendEvent("Unknown error storing model", "error");
        lastEnrollResult = "Store failed";
    }

    enrollmentInProgress = false;
    vTaskDelete(NULL);
}

// ---------- Web routes ----------

void handleList(AsyncWebServerRequest *request)
{
    // Return JSON array of IDs that exist
    String json = "[";
    for (int id = 1; id <= 127; ++id)
    {
        if (fingerprintExists(id))
        {
            json += String(id) + ",";
        }
    }
    if (json.endsWith(","))
        json.remove(json.length() - 1);
    json += "]";
    request->send(200, "application/json", json);
}

void handleEnroll(AsyncWebServerRequest *request)
{
    if (!request->hasParam("id"))
    {
        request->send(400, "text/plain", "Missing id param");
        return;
    }
    int id = request->getParam("id")->value().toInt();
    if (id < 1 || id > 127)
    {
        request->send(400, "text/plain", "ID must be 1..127");
        return;
    }

    if (enrollmentInProgress)
    {
        request->send(409, "text/plain", "Another enrollment is in progress");
        return;
    }

    // allocate id parameter for the task
    int *taskParam = (int *)malloc(sizeof(int));
    *taskParam = id;

    // create a FreeRTOS task to run enrollment to avoid blocking the server connection
    BaseType_t ok = xTaskCreate(
        enrollmentTask,
        "enrollTask",
        4096, // stack
        (void *)taskParam,
        1, // priority
        NULL);

    if (ok == pdPASS)
    {
        request->send(200, "text/plain", "Enrollment started");
    }
    else
    {
        free(taskParam);
        request->send(500, "text/plain", "Failed to start enrollment task");
    }
}

// ---------- HTML page ----------
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>ESP32 Fingerprint Manager</title>
<style>
body{font-family:system-ui,Arial;margin:20px}
#list{margin:8px 0;padding:0}
li{list-style:none;padding:4px 0}
button{padding:8px 14px}
#events{white-space:pre-wrap;background:#f7f7f7;padding:10px;border:1px solid #ddd;min-height:80px}
.prompt{color:darkorange}
.status{color:teal}
.error{color:crimson}
.done{color:green}
</style>
</head>
<body>
<h2>Fingerprint Manager</h2>

<section>
  <button onclick="refreshList()">Refresh list</button>
  <ul id="list"></ul>
</section>

<section style="margin-top:12px">
  <label>Enroll ID (1-127): <input id="enrollId" type="number" min="1" max="127" value="1"></label>
  <button onclick="startEnroll()">Enroll</button>
</section>

<h3>Events</h3>
<div id="events"></div>

<script>
const evtSource = new EventSource('/events');
evtSource.addEventListener('status', e => appendEvent(e.data, 'status'));
evtSource.addEventListener('prompt', e => appendEvent(e.data, 'prompt'));
evtSource.addEventListener('error', e => appendEvent(e.data, 'error'));
evtSource.addEventListener('done', e => { appendEvent(e.data, 'done'); refreshList(); });
evtSource.onopen = () => appendEvent('Connected to server','status');
evtSource.onerror = () => appendEvent('SSE connection error','error');

function appendEvent(msg, kind='status') {
  const node = document.createElement('div');
  node.textContent = msg;
  node.className = kind;
  document.getElementById('events').prepend(node);
}

function refreshList() {
  fetch('/fp/list').then(r=>r.json()).then(arr=>{
    const list = document.getElementById('list');
    list.innerHTML = arr.length ? arr.map(i=>`<li>ID ${i}</li>`).join('') : '<li>(none)</li>';
  });
}

function startEnroll(){
  const id = document.getElementById('enrollId').value;
  fetch(`/fp/enroll?id=${id}`)
    .then(r => r.text())
    .then(txt => appendEvent(txt, 'status'))
    .catch(err => appendEvent('Request failed: ' + err, 'error'));
}

// initial load
refreshList();
</script>
</body>
</html>
)rawliteral";

// ---------- Setup ----------
void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("Booting...");

    // start UART for fingerprint (57600)
    FingerSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
    delay(100);
    finger.begin(57600);
    if (finger.verifyPassword())
    {
        Serial.println("Fingerprint sensor found");
    }
    else
    {
        Serial.println("Fingerprint sensor not found - check wiring");
        // continue anyway; endpoints will fail if sensor unreachable
    }

    // Configure Access Point
    Serial.println("Configuring Access Point...");

    // Configure soft AP with custom IP
    if (!WiFi.softAPConfig(local_IP, gateway, subnet))
    {
        Serial.println("AP Config Failed!");
    }

    // Start Access Point
    bool apStarted;
    if (strlen(ap_password) >= 8)
    {
        // WPA2 secured network
        apStarted = WiFi.softAP(ap_ssid, ap_password);
    }
    else
    {
        // Open network (no password)
        apStarted = WiFi.softAP(ap_ssid);
    }

    if (apStarted)
    {
        Serial.println("Access Point started");
        Serial.print("AP SSID: ");
        Serial.println(ap_ssid);
        if (strlen(ap_password) >= 8)
        {
            Serial.print("AP Password: ");
            Serial.println(ap_password);
        }
        else
        {
            Serial.println("AP is open (no password)");
        }
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.println("Connect to the AP and navigate to http://192.168.4.1");
    }
    else
    {
        Serial.println("Failed to start Access Point!");
    }

    // Add routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", index_html); });

    server.on("/fp/list", HTTP_GET, handleList);
    server.on("/fp/enroll", HTTP_GET, handleEnroll);

    server.addHandler(&events); // SSE
    server.begin();
    Serial.println("Server started");
}

void loop()
{
    // nothing here, everything handled by server and FreeRTOS tasks
}