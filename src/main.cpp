#include <Arduino.h>

//* Define Pins
// Ultrasonic Pinouts
#define ust_left_trig 27
#define ust_left_echo 26
#define ust_right_trig 25
#define ust_right_echo 33

#define usp_left_trig 15
#define usp_left_echo 2
#define usp_right_trig 22
#define usp_right_echo 23

// Traffic Light Pinouts
#define tf1_red 18
#define tf1_yellow 19
#define tf1_green 21

// Pedestrian Light Pinouts
#define pd_red 16
#define pd_green 4

// Alarm
#define buzzer 32

float left_distance;
float right_distance;
float left_pedestrian_distance;
float right_pedestrian_distance;

// Define Functions
float getUltrasonicDistance(int trigPin, int echoPin);

void trafficLightTask(void *parameter);
void buzzerTask(void *parameter);

// Task Handles
TaskHandle_t trafficLightTaskHandle;
TaskHandle_t buzzerTaskHandle;

volatile bool isRedLightOn = false;

void setup() {
    Serial.begin(115200);

    // Ultrasonic Initialize
    pinMode(ust_left_trig, OUTPUT);
    pinMode(ust_left_echo, INPUT);
    pinMode(ust_right_trig, OUTPUT);
    pinMode(ust_right_echo, INPUT);

    pinMode(usp_left_trig, OUTPUT);
    pinMode(usp_left_echo, INPUT);
    pinMode(usp_right_trig, OUTPUT);
    pinMode(usp_right_echo, INPUT);

    // Traffic Initialize
    pinMode(tf1_red, OUTPUT);
    pinMode(tf1_yellow, OUTPUT);
    pinMode(tf1_green, OUTPUT);

    // Pedestrian Initialize
    pinMode(pd_green, OUTPUT);
    pinMode(pd_red, OUTPUT);

    // Buzzer Initialize
    pinMode(buzzer, OUTPUT);

    // Set default states
    digitalWrite(tf1_red, HIGH);
    digitalWrite(tf1_yellow, HIGH);
    digitalWrite(tf1_green, HIGH);
    digitalWrite(pd_green, HIGH);
    digitalWrite(pd_red, HIGH);
    digitalWrite(buzzer, HIGH);

    // Create tasks
    xTaskCreatePinnedToCore(trafficLightTask, "Traffic Light Task", 2048, NULL, 1, &trafficLightTaskHandle, 0);
    xTaskCreatePinnedToCore(buzzerTask, "Buzzer Task", 1024, NULL, 1, &buzzerTaskHandle, 1);
}

void loop() {
    // Main loop handles ultrasonic readings
    left_distance = getUltrasonicDistance(ust_left_trig, ust_left_echo);
    right_distance = getUltrasonicDistance(ust_right_trig, ust_right_echo);
    left_pedestrian_distance = getUltrasonicDistance(usp_left_trig, usp_left_echo);
    right_pedestrian_distance = getUltrasonicDistance(usp_right_trig, usp_right_echo);

    delay(100);  // Reduce loop execution speed
}

void trafficLightTask(void *parameter) {
    while (true) {
        // Conditions
        bool leftDistanceCondition = left_distance >= 0.25 && left_distance <= 2.25;
        bool rightDistanceCondition = right_distance >= 0.25 && right_distance <= 2.25;

        if (leftDistanceCondition || rightDistanceCondition) {
            // Red Light (Stop)
            isRedLightOn = true;

            Serial.println("Red Light - Vehicles Stop, Pedestrians Go");

            digitalWrite(tf1_red, LOW);  // Red light ON
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, HIGH);

            digitalWrite(pd_green, LOW);  // Pedestrian green ON
            digitalWrite(pd_red, HIGH);   // Pedestrian red OFF

            vTaskDelay(pdMS_TO_TICKS(15000));  // Red light ON for 30 seconds

            // Prepare to Go (Red and Yellow)
            isRedLightOn = false;

            Serial.println("Red and Yellow Light - Prepare to Go");
            digitalWrite(tf1_red, LOW);
            digitalWrite(tf1_yellow, LOW);  // Yellow ON
            digitalWrite(tf1_green, HIGH);

            digitalWrite(pd_green, HIGH);  // Pedestrian green OFF
            digitalWrite(pd_red, LOW);     // Pedestrian red ON

            vTaskDelay(pdMS_TO_TICKS(5000));  // Red and Yellow ON for 5 seconds

            // Green Light (Go)
            Serial.println("Green Light - Vehicles Go, Pedestrians Stop");

            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, LOW);  // Green ON

            digitalWrite(pd_green, HIGH);  // Pedestrian green OFF
            digitalWrite(pd_red, LOW);     // Pedestrian red ON

            vTaskDelay(pdMS_TO_TICKS(15000));  // Green light ON for 30 seconds

            // Yellow Light (Prepare to Stop)
            Serial.println("Yellow Light - Prepare to Stop");
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, LOW);  // Yellow ON
            digitalWrite(tf1_green, HIGH);

            digitalWrite(pd_green, HIGH);
            digitalWrite(pd_red, LOW);
            vTaskDelay(pdMS_TO_TICKS(5000));  // Green light ON for 30 seconds

        } else {
            Serial.println("Condition not met, setting default state.");
            isRedLightOn = false;

            // Turn off all lights
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, HIGH);

            digitalWrite(pd_green, HIGH);  // Pedestrian green OFF
            digitalWrite(pd_red, HIGH);    // Pedestrian red OFF
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // Short delay to prevent rapid toggling
    }
}

void buzzerTask(void *parameter) {
    while (true) {
        if (isRedLightOn && (left_pedestrian_distance <= 2.25 || right_pedestrian_distance <= 2.25)) {
            Serial.println("Pedestrian detected during red light - Activating buzzer.");
            digitalWrite(buzzer, LOW);       // Buzzer ON
            vTaskDelay(pdMS_TO_TICKS(500));  // ON for 500 ms
            digitalWrite(buzzer, HIGH);      // Buzzer OFF
            vTaskDelay(pdMS_TO_TICKS(500));  // OFF for 500 ms
        } else {
            digitalWrite(buzzer, HIGH);      // Ensure buzzer is OFF
            vTaskDelay(pdMS_TO_TICKS(100));  // Check condition periodically
        }
    }
}

float getUltrasonicDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = (duration * 0.034) / 2.0;

    return distance / 100.0;  // Convert to meters
}
