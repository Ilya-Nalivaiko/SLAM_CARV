#!/bin/bash

# Building Script for CARV SLAM docker container

clear
echo "========================================================================================================="
echo "> MAINTAINER: Islam A. Ali <islam.a.mustafa@gmail.com>"
echo "> VERSION: 1.0"
echo "========================================================================================================="

                                                                                                         
DOCKER_BUILDKIT=1 docker build --progress=plain -t islamaali/slam_carv-docker:v1.0 -f Docker/Dockerfile .