#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// --- FILL THESE IN ---
const char* ssid       = "YOUR_WIFI";
const char* password   = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "YOUR_BROKER_IP";
const char* mqttUser   = "open_sesame";
const char* mqttPass   = "YOUR_MQTT_PASSWORD";

// --- SERVO ---
const int SERVO_PIN = 13;
Servo doorServo;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];

    Serial.println("Received: " + message);

    if (message == "unlock") {
        Serial.println("Face detected — sweeping to 45°");
        doorServo.write(45);
        delay(3000);           // hold for 3 seconds
        doorServo.write(0);
        Serial.println("Returned to 0°");
    }
}

void setup() {
    Serial.begin(115200);
    doorServo.attach(SERVO_PIN);
    doorServo.write(0);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        while (!client.connect("esp32-poc", mqttUser, mqttPass)) {
            delay(500);
            Serial.print(".");
        }
        client.subscribe("door/lock/command");
        Serial.println("MQTT connected, waiting for face detection...");
    }
    client.loop();
}
