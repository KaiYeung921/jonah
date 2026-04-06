FROM python:3.13-slim
WORKDIR /usr/local/app

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY main.py .
COPY known_faces/ known_faces/

RUN useradd app
USER app

CMD ["python3","main.py"]

