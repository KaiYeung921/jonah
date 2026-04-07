#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid       = "Claremont-ETC";
const char* password   = "Cl@remontI0T";
const char* mqttServer = "172.28.121.73";
const char* mqttUser   = "jonah";
const char* mqttPass   = "jfr";

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
        Serial.println("Face detected — sweeping to 52°");
        doorServo.write(52);
        delay(3000);
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
