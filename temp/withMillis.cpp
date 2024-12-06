#include <Arduino.h>

// Define Pins
// Ultrasonic Pinouts
#define ust_left_trig 27
#define ust_left_echo 26
#define ust_right_trig 25
#define ust_right_echo 33

// Traffic Light Pinouts
#define tf1_red 21
#define tf1_yellow 19
#define tf1_green 18

// Pedestrian Light Pinouts
#define pd_red 16
#define pd_green 4

// Define Timing
unsigned long previousMillis = 0;    // Stores the last time a light changed
unsigned long lightDuration = 5000;  // Duration for each light (in ms)

// Timing variables for each light state
enum TrafficLightState { RED,
                         RED_YELLOW,
                         GREEN,
                         YELLOW };
TrafficLightState currentState = RED;

// Define Functions
float getUltrasonicDistance(int trigPin, int echoPin);
void updateTrafficLights();

void setup() {
    Serial.begin(115200);

    // Ultrasonic Initialize
    pinMode(ust_left_trig, OUTPUT);
    pinMode(ust_left_echo, INPUT);
    pinMode(ust_right_trig, OUTPUT);
    pinMode(ust_right_echo, INPUT);

    // Traffic Initialize
    pinMode(tf1_red, OUTPUT);
    pinMode(tf1_yellow, OUTPUT);
    pinMode(tf1_green, OUTPUT);

    // Pedestrian Initialize
    pinMode(pd_green, OUTPUT);
    pinMode(pd_red, OUTPUT);
}

void loop() {
    // Get the current time
    unsigned long currentMillis = millis();

    // Read the ultrasonic distance
    float left_distance = getUltrasonicDistance(ust_left_trig, ust_left_echo);
    bool leftDistanceCondition = left_distance <= 1.0;  // Change condition if needed

    // Debug print the left distance
    Serial.print("Left Ultrasonic Distance: ");
    Serial.println(left_distance);
    Serial.print("Left Side Vehicle: ");
    Serial.println(leftDistanceCondition ? "Stop" : "Go");

    if (leftDistanceCondition) {
        // Update the traffic lights based on the time passed
        updateTrafficLights();
    } else {
        // Default behavior when no vehicle is detected
        digitalWrite(tf1_red, HIGH);    // Red ON for vehicles
        digitalWrite(tf1_yellow, LOW);  // Yellow OFF
        digitalWrite(tf1_green, LOW);   // Green OFF
        digitalWrite(pd_red, LOW);      // Pedestrian Red OFF
        digitalWrite(pd_green, HIGH);   // Pedestrian Green ON
    }

    // Update other logic, sensor checks, or buttons here
}

void updateTrafficLights() {
    unsigned long currentMillis = millis();

    switch (currentState) {
        case RED:
            // Red Light (Stop)
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, LOW);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, LOW);
            digitalWrite(pd_green, HIGH);  // Pedestrian Green ON
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = RED_YELLOW;
            }
            break;

        case RED_YELLOW:
            // Red and Yellow (Prepare to Go)
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = GREEN;
            }
            break;

        case GREEN:
            // Green Light (Go)
            digitalWrite(tf1_red, LOW);
            digitalWrite(tf1_yellow, LOW);
            digitalWrite(tf1_green, HIGH);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = YELLOW;
            }
            break;

        case YELLOW:
            // Yellow Light (Prepare to Stop)
            digitalWrite(tf1_red, LOW);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = RED;  // Reset to Red
            }
            break;
    }
}

float getUltrasonicDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float initialDistance = duration * 0.034 / 2.0;
    float distanceinMeter = initialDistance / 100;
    return distanceinMeter;
}
#include <Arduino.h>

// Define Pins
// Ultrasonic Pinouts
#define ust_left_trig 27
#define ust_left_echo 26
#define ust_right_trig 25
#define ust_right_echo 33

// Traffic Light Pinouts
#define tf1_red 21
#define tf1_yellow 19
#define tf1_green 18

// Pedestrian Light Pinouts
#define pd_red 16
#define pd_green 4

// Define Timing
unsigned long previousMillis = 0;    // Stores the last time a light changed
unsigned long lightDuration = 5000;  // Duration for each light (in ms)

// Timing variables for each light state
enum TrafficLightState { RED,
                         RED_YELLOW,
                         GREEN,
                         YELLOW };
TrafficLightState currentState = RED;

// Define Functions
float getUltrasonicDistance(int trigPin, int echoPin);
void updateTrafficLights();

void setup() {
    Serial.begin(115200);

    // Ultrasonic Initialize
    pinMode(ust_left_trig, OUTPUT);
    pinMode(ust_left_echo, INPUT);
    pinMode(ust_right_trig, OUTPUT);
    pinMode(ust_right_echo, INPUT);

    // Traffic Initialize
    pinMode(tf1_red, OUTPUT);
    pinMode(tf1_yellow, OUTPUT);
    pinMode(tf1_green, OUTPUT);

    // Pedestrian Initialize
    pinMode(pd_green, OUTPUT);
    pinMode(pd_red, OUTPUT);
}

void loop() {
    // Get the current time
    unsigned long currentMillis = millis();

    // Read the ultrasonic distance
    float left_distance = getUltrasonicDistance(ust_left_trig, ust_left_echo);
    bool leftDistanceCondition = left_distance <= 1.0;  // Change condition if needed

    // Debug print the left distance
    Serial.print("Left Ultrasonic Distance: ");
    Serial.println(left_distance);
    Serial.print("Left Side Vehicle: ");
    Serial.println(leftDistanceCondition ? "Stop" : "Go");

    if (leftDistanceCondition) {
        // Update the traffic lights based on the time passed
        updateTrafficLights();
    } else {
        // Default behavior when no vehicle is detected
        digitalWrite(tf1_red, HIGH);    // Red ON for vehicles
        digitalWrite(tf1_yellow, LOW);  // Yellow OFF
        digitalWrite(tf1_green, LOW);   // Green OFF
        digitalWrite(pd_red, LOW);      // Pedestrian Red OFF
        digitalWrite(pd_green, HIGH);   // Pedestrian Green ON
    }

    // Update other logic, sensor checks, or buttons here
}

void updateTrafficLights() {
    unsigned long currentMillis = millis();

    switch (currentState) {
        case RED:
            // Red Light (Stop)
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, LOW);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, LOW);
            digitalWrite(pd_green, HIGH);  // Pedestrian Green ON
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = RED_YELLOW;
            }
            break;

        case RED_YELLOW:
            // Red and Yellow (Prepare to Go)
            digitalWrite(tf1_red, HIGH);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = GREEN;
            }
            break;

        case GREEN:
            // Green Light (Go)
            digitalWrite(tf1_red, LOW);
            digitalWrite(tf1_yellow, LOW);
            digitalWrite(tf1_green, HIGH);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = YELLOW;
            }
            break;

        case YELLOW:
            // Yellow Light (Prepare to Stop)
            digitalWrite(tf1_red, LOW);
            digitalWrite(tf1_yellow, HIGH);
            digitalWrite(tf1_green, LOW);
            digitalWrite(pd_red, HIGH);   // Pedestrian Red ON
            digitalWrite(pd_green, LOW);  // Pedestrian Green OFF
            if (currentMillis - previousMillis >= lightDuration) {
                previousMillis = currentMillis;
                currentState = RED;  // Reset to Red
            }
            break;
    }
}

float getUltrasonicDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float initialDistance = duration * 0.034 / 2.0;
    float distanceinMeter = initialDistance / 100;
    return distanceinMeter;
}
