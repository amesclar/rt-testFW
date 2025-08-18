FROM python:3.9.19-slim-bullseye
# FROM alpine:3.20
# FROM python:3.9

RUN useradd -ms /bin/bash cfa
USER cfa
WORKDIR /home/cfa

COPY . /app

WORKDIR /app

RUN pip install --upgrade pip && pip install -r requirements.txt

# CMD ["python", "app.py"]
# CMD pwd
