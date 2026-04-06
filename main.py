#!/usr/bin/env python3
import cv2
from deepface import DeepFace
import time
import threading
import paho.mqtt.client as mqtt
from dotenv import load_dotenv
import os

load_dotenv()

# --- CONFIG ---
AUTHORIZED = {
    "kai":   "known_faces/Kai/Kai.jpg",
    "misha": "known_faces/Misha/Misha.jpg",
}
MODEL          = "ArcFace"
CHECK_INTERVAL = 7    # seconds between face scans
LOCK_DELAY     = 5    # seconds after door closes before re-locking
LOCK_TIMEOUT   = 30   # fallback: re-lock this many seconds after unlock if reed never fires

DRY_RUN = True  # set False when connected to broker

# --- MQTT topics (must match firmware.cpp) ---
TOPIC_LOCK_COMMAND   = "door/lock/command"
TOPIC_LOCK_STATE     = "door/lock/state"
TOPIC_FACE_DETECTION = "door/detection/face"
TOPIC_REED           = "door/sensor/reed"

# --- MQTT credentials (loaded from .env) ---
MQTT_BROKER = os.getenv("MQTT_BROKER")
MQTT_PORT   = int(os.getenv("MQTT_PORT", 1883))
MQTT_USER   = os.getenv("MQTT_USER")
MQTT_PASS   = os.getenv("MQTT_PASS")

# --- State ---
door_unlocked = False
relock_timer  = None

def relock():
    global door_unlocked, relock_timer
    publish(TOPIC_LOCK_COMMAND, "lock")
    door_unlocked = False
    relock_timer  = None
    print("Relocked")

def schedule_relock(delay):
    """Cancel any pending relock and schedule a new one."""
    global relock_timer
    if relock_timer:
        relock_timer.cancel()
    relock_timer = threading.Timer(delay, relock)
    relock_timer.start()

# --- MQTT callbacks ---
def on_lock_state(client, userdata, msg):
    print(f"[lock state] {msg.payload.decode()}")

def on_reed(client, userdata, msg):
    state = msg.payload.decode()
    print(f"[reed] {state}")
    # Door just closed after being unlocked → start re-lock countdown
    if state == "closed" and door_unlocked:
        schedule_relock(LOCK_DELAY)

if not DRY_RUN:
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.message_callback_add(TOPIC_LOCK_STATE, on_lock_state)
    client.message_callback_add(TOPIC_REED, on_reed)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.subscribe(TOPIC_LOCK_STATE)
    client.subscribe(TOPIC_REED)
    client.loop_start()

def publish(topic, message):
    if DRY_RUN:
        print(f"[DRY RUN] would publish → {topic}: {message}")
    else:
        client.publish(topic, message)

# --- Camera ---
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
                recognized    = True
                door_unlocked = True
                publish(TOPIC_FACE_DETECTION, f'{{"match": true, "identity": "{name}"}}')
                publish(TOPIC_LOCK_COMMAND, "unlock")
                # Fallback relock if reed switch never reports door closed
                schedule_relock(LOCK_TIMEOUT)
                break

        except Exception as e:
            print(f"Error checking {name}: {e}")

    if not recognized:
        print("Denied")
        publish(TOPIC_FACE_DETECTION, '{"match": false}')

cap.release()
cv2.destroyAllWindows()
