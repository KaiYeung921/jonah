#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// --- CONFIG ---
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "THINKPAD_IP";
const char* mqtt_user = "open_sesame";
const char* mqtt_pass = "your_password_here";

const int SERVO_PIN = 13;
const int LOCKED_ANGLE = 0;
const int UNLOCKED_ANGLE = 45;  // tune this for your handle

// --- OBJECTS ---
WiFiClient espClient;
PubSubClient client(espClient);
Servo doorServo;

// --- CALLBACK ---
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];

    Serial.println("Message received: " + message);

    if (String(topic) == "door/lock/command") {
        if (message == "unlock") {
            doorServo.write(UNLOCKED_ANGLE);
            client.publish("door/lock/state", "unlocked");
            Serial.println("Unlocked");
        } else if (message == "lock") {
            doorServo.write(LOCKED_ANGLE);
            client.publish("door/lock/state", "locked");
            Serial.println("Locked");
        }
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);

    doorServo.attach(SERVO_PIN);
    doorServo.write(LOCKED_ANGLE);

    // Connect WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

    // Connect MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

// --- LOOP ---
void loop() {
    if (!client.connected()) {
        Serial.println("MQTT disconnected, reconnecting...");
        while (!client.connect("esp32-door", mqtt_user, mqtt_pass)) {
            delay(500);
            Serial.print(".");
        }
        client.subscribe("door/lock/command");
        Serial.println("MQTT connected");
    }
    client.loop();
}