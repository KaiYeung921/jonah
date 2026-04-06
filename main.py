#!/usr/bin/env python3
import cv2
from deepface import DeepFace
import time
import paho.mqtt.client as mqtt

AUTHORIZED = {
    "kai": "known_faces/Kai/Kai.jpg",
    "misha": "known_faces/Misha/Misha.jpg",
}
MODEL = "ArcFace"
CHECK_INTERVAL = 7  # seconds between scans

MQTT_BROKER = "Thinkpad IP"
MQTT_PORT = 1883
MQTT_USER = 'open_sesame'
MQTT_PASS = "password_here"

DRY_RUN = True # change when connected to thinkpad & mosquitto ready

if not DRY_RUN:
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER,MQTT_PASS)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    
def publish(topic, message):
    if DRY_RUN:
        print(f"[DRY RUN] would publish → {topic}: {message}")
    else:
        client.publish(topic, message)
    

cap = cv2.VideoCapture(1)
last_check = 0

print("Running... press 'q' to quit")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Can't grab frame")
        break

    cv2.imshow("Face Lock", frame)
    key = cv2.waitKey(1) & 0xFF

    if key == ord('q'):
        break

    now = time.time()
    if now - last_check < CHECK_INTERVAL:
        continue
    last_check = now

    recognized = False
    for name, ref_img in AUTHORIZED.items():
        try:
            result = DeepFace.verify(
                img1_path=frame,
                img2_path=ref_img,
                model_name=MODEL,
                distance_metric="cosine",
                enforce_detection=False
            )
            distance = round(result["distance"], 3)
            print(f"  {name}: distance={distance} verified={result['verified']}")

            if result["verified"]:
                print(f"UNLOCKED: {name}")
                recognized = True
                publish("door/detection/face", f'{{"match" : true, "identity": "{name}"}}')
                publish("door/lock/command", "unlock")
                break

        except Exception as e:
            print(f"Error checking {name}: {e}")

    if not recognized:
        print("Denied")
        publish("door/detection/face", '{"match" : false}')

cap.release()
cv2.destroyAllWindows()