#!/bin/bash
                                                                                                
DOCKER_BUILDKIT=1 docker build --progress=plain -t islamaali/slam_carv-docker:v1.0 -f Docker/Dockerfile .