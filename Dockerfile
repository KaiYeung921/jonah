FROM python:3.13-slim
WORKDIR /usr/local/app

RUN apt-get update && apt-get install -y --no-install-recommends \
    libglib2.0-0 \
    libgl1 \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt && \
    pip install --no-cache-dir --force-reinstall opencv-python-headless

COPY main.py .
COPY known_faces/ known_faces/

RUN useradd -m app && usermod -aG video app
USER app

RUN python3 -c "from deepface import DeepFace; DeepFace.build_model('ArcFace')"

CMD ["python3","main.py"]


