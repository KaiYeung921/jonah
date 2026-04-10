#!/usr/bin/env python

import pyaudio
import numpy as np
from openwakeword.model import Model

model = Model()
print(model.models.keys())

pa = pyaudio.PyAudio()
stream = pa.open(
    rate=16000,
    channels=1,
    format=pyaudio.paInt16,
    input=True,
    frames_per_buffer=1280
)

print("Listening...")
while True:
    audio = stream.read(1280, exception_on_overflow=False)
    audio_np = np.frombuffer(audio, dtype=np.int16)

    prediction = model.predict(audio_np)

    scores = {k: f"{v:.3f}" for k, v in prediction.items() if v > 0.05}
    if scores:
        print(scores)
