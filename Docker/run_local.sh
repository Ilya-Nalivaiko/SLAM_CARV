#!/bin/bash

docker run \
  -p 5901:5901 \
  -p 8080:8080 \
  --tmpfs /tmp:rw,size=100m \
  --tmpfs /root/.vnc \
  -v ~/SLAM_CARV/shared:/root/orbslam3/SLAM_CARV/shared \
  --rm -it \
  --privileged \
  -e DISPLAY=:1 \
  -e QT_X11_NO_MITSHM=1 \
  --security-opt seccomp=unconfined \
  --security-opt apparmor=unconfined \
  --device=/dev/video0:/dev/video0 \
  islamaali/slam_carv-docker:v1.0 bash
