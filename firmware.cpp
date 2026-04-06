#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include "secrets.h"

// --- CONFIG ---
const int SERVO_PIN      = 13;
const int REED_PIN       = 14;
const int LOCKED_ANGLE   = 0;
const int UNLOCKED_ANGLE = 45;  // tune this for your handle

const char* MQTT_CLIENT_ID       = "esp32-actuator";
const char* TOPIC_LOCK_COMMAND   = "door/lock/command";
const char* TOPIC_LOCK_STATE     = "door/lock/state";
const char* TOPIC_FACE_DETECTION = "door/detection/face";
const char* TOPIC_REED           = "door/sensor/reed";

// --- OBJECTS ---
WiFiClient espClient;
PubSubClient client(espClient);
Servo doorServo;

// --- CALLBACK ---
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];

    Serial.println("Message received on " + String(topic) + ": " + message);

    if (String(topic) == TOPIC_LOCK_COMMAND) {
        if (message == "unlock") {
            doorServo.write(UNLOCKED_ANGLE);
            client.publish(TOPIC_LOCK_STATE, "unlocked");
            Serial.println("Unlocked");
        } else if (message == "lock") {
            doorServo.write(LOCKED_ANGLE);
            client.publish(TOPIC_LOCK_STATE, "locked");
            Serial.println("Locked");
        }
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);

    doorServo.attach(SERVO_PIN);
    doorServo.write(LOCKED_ANGLE);
    pinMode(REED_PIN, INPUT_PULLUP);

    // Connect WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

    // Connect MQTT
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
}

// --- LOOP ---
void loop() {
    if (!client.connected()) {
        Serial.println("MQTT disconnected, reconnecting...");
        while (!client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
            delay(500);
            Serial.print(".");
        }
        client.subscribe(TOPIC_LOCK_COMMAND);
        Serial.println("MQTT connected");
    }
    client.loop();

    // Publish reed switch state changes
    static int lastReed = -1;
    int reed = digitalRead(REED_PIN);
    if (reed != lastReed) {
        client.publish(TOPIC_REED, reed == LOW ? "closed" : "open");
        lastReed = reed;
    }
}
