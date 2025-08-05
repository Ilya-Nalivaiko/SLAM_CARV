#!/bin/bash

docker run -d \
  -p 5901:5901 \
  -p 8080:8080 \
  --tmpfs /tmp:rw,size=100m \
  --tmpfs /root/.vnc \
  --rm \
  --privileged \
  --device=/dev/video0:/dev/video0 \
  -e DISPLAY=:1 \
  -e QT_X11_NO_MITSHM=1 \
  -it islamaali/slam_carv-docker:v1.0
