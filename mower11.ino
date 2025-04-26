/*
   ESP32 Autonomous Lawn Mower with Manual Web Control
   - Controls mower via a web interface
   - Uses three ultrasonic sensors for obstacle detection
   - Features manual and autonomous modes
*/

#include <WiFi.h>
#include <WebServer.h>
#include <NewPing.h>
// #include <ESP32Servo.h>

// Wi-Fi Credentials or Device Name Code
const char *ssid = "SmartMOWER";
const char *password = "";

// Web Server on port 80
WebServer server(80);

// Define Motor Control Pins
#define DRIVE_IN1 26
#define DRIVE_IN2 27
#define STEERING_IN1 14
#define STEERING_IN2 12

// Define Ultrasonic Sensor Pins
#define TRIGGER_FRONT 5
#define ECHO_FRONT 18
#define TRIGGER_LEFT 23
#define ECHO_LEFT 19
#define TRIGGER_RIGHT 25
#define ECHO_RIGHT 21

// Define Servo Motor Pins
#define STEERING_SERVO 33 //Front
#define SERVO_LEFT 32
#define SERVO_RIGHT 15

// Obstacle avoidance threshold (can be adjusted)
const int OBSTACLE_DISTANCE = 60;

// Instantiate Ultrasonic Sensors
NewPing frontSensor(TRIGGER_FRONT, ECHO_FRONT, 200);
NewPing leftSensor(TRIGGER_LEFT, ECHO_LEFT, 200);
NewPing rightSensor(TRIGGER_RIGHT, ECHO_RIGHT, 200);

// Control Mode: "manual" or "auto"
String controlMode = "manual";

// Function to control the mower's movement
void moveMower(String direction) {
    if (direction == "forward") {
        digitalWrite(DRIVE_IN1, LOW);
        digitalWrite(DRIVE_IN2, HIGH);
    } else if (direction == "backward") {
        digitalWrite(DRIVE_IN1, HIGH);
        digitalWrite(DRIVE_IN2, LOW);
    } else if (direction == "left") {
        digitalWrite(STEERING_IN1, HIGH);
        digitalWrite(STEERING_IN2, LOW);
    } else if (direction == "right") {
        digitalWrite(STEERING_IN1, LOW);
        digitalWrite(STEERING_IN2, HIGH);
    } else if (direction == "stop") {
        digitalWrite(DRIVE_IN1, LOW);
        digitalWrite(DRIVE_IN2, LOW);
        digitalWrite(STEERING_IN1, LOW);
        digitalWrite(STEERING_IN2, LOW);
    }
}

// Handle manual commands from the web interface
void handleCommand() {
    if (server.hasArg("command")) {
        String command = server.arg("command");
        command.toLowerCase();
        Serial.print("Received Command: ");
        Serial.println(command);

        if (controlMode == "manual") {
            moveMower(command);
            server.send(200, "text/plain", "Command executed: " + command);
        } else {
            server.send(403, "text/plain", "Manual control is disabled in autonomous mode.");
        }
    } else {
        server.send(400, "text/plain", "Missing command parameter.");
    }
}

// Handle mode switching
void handleModeSwitch() {
    if (server.hasArg("mode")) {
        String mode = server.arg("mode");
        mode.toLowerCase();

        if (mode == "manual" || mode == "auto") {
            controlMode = mode;
            server.send(200, "text/plain", "Mode switched to: " + controlMode);
            Serial.println("Mode switched to: " + controlMode);
        } else {
            server.send(400, "text/plain", "Invalid mode. Use 'manual' or 'auto'.");
        }
    } else {
        server.send(400, "text/plain", "Missing mode parameter.");
    }
}

void scanAndNavigate() {
    int frontDist = frontSensor.ping_cm();
    int leftDist = leftSensor.ping_cm();
    int rightDist = rightSensor.ping_cm();

    // Avoid false zero readings
    if (frontDist == 0) frontDist = OBSTACLE_DISTANCE + 10;
    if (leftDist == 0) leftDist = OBSTACLE_DISTANCE + 10;
    if (rightDist == 0) rightDist = OBSTACLE_DISTANCE + 10;

    Serial.print("Front: "); Serial.print(frontDist); Serial.print(" cm, ");
    Serial.print("Left: "); Serial.print(leftDist); Serial.print(" cm, ");
    Serial.print("Right: "); Serial.println(rightDist); Serial.print(" cm, ");

    // Navigation logic with increased speed
    if (frontDist <= OBSTACLE_DISTANCE || leftDist <= OBSTACLE_DISTANCE || rightDist <= OBSTACLE_DISTANCE) {
        moveMower("stop");
        Serial.println("Stop");
        delay(900);

        if (leftDist > rightDist) {
            moveMower("right");
            Serial.println("Turning Right");
            moveMower("backward");
            Serial.println("Moving Backward");
            delay(1200);
            moveMower("left");
            Serial.println("Turning Left");
            moveMower("right");
            Serial.println("Turning Right");
            
        } else {
            moveMower("left");
            Serial.println("Turning Left");
            moveMower("backward");
            delay(1200);
            Serial.println("Moving Backward");
            moveMower("right");
            Serial.println("Turning Right");
            moveMower("left");
            Serial.println("Turning Left");
        }   
    } else {
        moveMower("forward");
        Serial.println("Moving Forward");
    }
}


void setup() {
    Serial.begin(115200);
    WiFi.softAP(ssid, password);
    Serial.print("ESP32 AP IP: ");
    Serial.println(WiFi.softAPIP());

    pinMode(DRIVE_IN1, OUTPUT);
    pinMode(DRIVE_IN2, OUTPUT);
    pinMode(STEERING_IN1, OUTPUT);
    pinMode(STEERING_IN2, OUTPUT);

    server.on("/control", HTTP_GET, handleCommand);
    server.on("/mode", HTTP_GET, handleModeSwitch);
    server.begin();
    Serial.println("HTTP Server Started");
}

void loop() {
    server.handleClient();

    if (controlMode == "auto") {
        scanAndNavigate();
    }
}
