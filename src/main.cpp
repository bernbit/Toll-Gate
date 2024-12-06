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
#define tf1_red 21
#define tf1_yellow 19
#define tf1_green 18

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

    digitalWrite(tf1_red, HIGH);
    digitalWrite(tf1_yellow, HIGH);
    digitalWrite(tf1_green, HIGH);
    digitalWrite(pd_green, HIGH);
    digitalWrite(pd_red, HIGH);
    digitalWrite(buzzer, HIGH);
}

void loop() {
    left_distance = getUltrasonicDistance(ust_left_trig, ust_left_echo);
    right_distance = getUltrasonicDistance(ust_right_trig, ust_right_echo);

    left_pedestrian_distance = getUltrasonicDistance(usp_left_trig, usp_left_echo);
    right_pedestrian_distance = getUltrasonicDistance(usp_right_trig, usp_left_echo);

    // Conditions
    bool leftDistanceCondition = left_distance >= 0.25 && left_distance <= 2.25;
    bool rightDistanceCondition = right_distance >= 0.25 && right_distance <= 2.25;
    bool leftPedestrianCondition = left_pedestrian_distance >= 0.25 && left_pedestrian_distance <= 2.25;
    bool rightPedestrianCondition = right_pedestrian_distance >= 0.25 && right_pedestrian_distance <= 2.25;

    Serial.print("Left Ultrasonic US1 Distance: ");
    Serial.println(left_distance);

    if (leftDistanceCondition || rightDistanceCondition) {
        // Red Light (Stop)
        Serial.println("Red Light");
        Serial.println("Left Side Vehicle: Stop");
        Serial.println("Pedestrian: Go");

        // Blinking Red Light
        for (int i = 0; i < 10; i++) {
            digitalWrite(tf1_red, LOW);
            digitalWrite(buzzer, LOW);
            delay(150);
            digitalWrite(tf1_red, HIGH);
            digitalWrite(buzzer, HIGH);
            delay(150);
        }

        digitalWrite(tf1_red, LOW);      // Turn ON red light
        digitalWrite(tf1_yellow, HIGH);  // Turn OFF yellow light
        digitalWrite(tf1_green, HIGH);   // Turn OFF green light

        digitalWrite(pd_green, LOW);
        digitalWrite(pd_red, HIGH);

        // Buzzer logic
        if (digitalRead(tf1_red) == LOW && digitalRead(pd_green) == LOW &&
            (leftPedestrianCondition || rightPedestrianCondition)) {
            digitalWrite(buzzer, LOW);   // Activate buzzer
            delay(5000);                 // Buzzer ON for 5 seconds
            digitalWrite(buzzer, HIGH);  // Turn OFF buzzer
        }

        delay(30000);  // Stay on red for 30 seconds

        // Red and Amber (Prepare to Go)
        Serial.println("Red and Yellow Light");
        Serial.println("Left Side Vehicle: Prepare to Go");
        Serial.println("Pedestrian: Stop");
        digitalWrite(tf1_red, LOW);     // Keep red light ON
        digitalWrite(tf1_yellow, LOW);  // Turn ON yellow light
        digitalWrite(tf1_green, HIGH);  // Keep green light OFF

        digitalWrite(pd_green, HIGH);
        digitalWrite(pd_red, LOW);
        delay(5000);  // Stay in this state for 5 seconds

        // Green Light (Go)
        Serial.println("Green Light");
        Serial.println("Left Side Vehicle: Go");
        Serial.println("Pedestrian: Stop");
        digitalWrite(tf1_red, HIGH);     // Turn OFF red light
        digitalWrite(tf1_yellow, HIGH);  // Turn OFF yellow light
        digitalWrite(tf1_green, LOW);    // Turn ON green light

        digitalWrite(pd_green, HIGH);
        digitalWrite(pd_red, LOW);
        delay(30000);  // Stay on green for 30 seconds

        // Amber Light (Prepare to Stop)
        Serial.println("Yellow Light");
        Serial.println("Left Side Vehicle: Prepare to Stop");
        Serial.println("Pedestrian: Stop");
        digitalWrite(tf1_red, HIGH);    // Keep red light OFF
        digitalWrite(tf1_yellow, LOW);  // Turn ON yellow light
        digitalWrite(tf1_green, HIGH);  // Turn OFF green light

        digitalWrite(pd_green, HIGH);
        digitalWrite(pd_red, LOW);
        delay(5000);  // Stay in this state for 5 seconds
    } else {
        Serial.println("Pedestrian: Go");
        digitalWrite(tf1_red, LOW);      // Red ON for vehicles
        digitalWrite(tf1_yellow, HIGH);  // Yellow OFF
        digitalWrite(tf1_green, HIGH);   // Green OFF
        digitalWrite(pd_red, HIGH);      // Pedestrian Red OFF
        digitalWrite(pd_green, LOW);     // Pedestrian Green ON
    }

    // if (left_pedestrian_distance || right_pedestrian_distance) {
    //     digitalWrite(buzzer, LOW);
    //     delay(10000);
    //     digitalWrite(buzzer, HIGH);
    // }
}

float getUltrasonicDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(10);
    digitalWrite(trigPin, HIGH);

    long duration = pulseIn(echoPin, HIGH);
    float initialDistance = duration * 0.034 / 2.0;
    // float distance = initialDistance - 100;
    float distanceinMeter = initialDistance / 100;

    delay(500);  // Add a mini delay (50 ms) before the next reading

    return distanceinMeter;
}
