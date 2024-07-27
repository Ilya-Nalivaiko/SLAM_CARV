#!/bin/bash

docker run -d -p 5900:5900 -m 4g --cpus="4.0" --rm --network host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --privileged --device=/dev/video0:/dev/video0 --env QT_X11_NO_MITSHM=1 -it islamaali/slam_carv-docker:v1.0
