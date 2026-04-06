# Jonah

Facial recognition door lock built with DeepFace + ESP32 + MQTT.

## How It Works
1. Camera detects face and runs ArcFace verification
2. Match found → publishes unlock command via MQTT
3. ESP32 receives command → servo pushes door handle

## Stack
- Python, OpenCV, DeepFace (ArcFace)
- MQTT (Mosquitto broker on ThinkPad)
- ESP32 + servo actuator 

## Setup
```bash
pip install -r requirements.txt
```

EX. Add reference photos:
known_faces/
├── kai/kai.jpg
└── misha/misha.jpg

