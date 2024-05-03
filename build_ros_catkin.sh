#!/usr/bin/env bash

echo "Building ROS nodes"

# Create a Catkin workspace
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src

# Copy your ROS nodes source code into the Catkin workspace
cp -r ~/orbslam3/SLAM_CARV/Examples/ROS/ORB_CARV_Pub/src/* .

# Navigate to the Catkin workspace
cd ~/catkin_ws

# Initialize Catkin workspace
catkin_init_workspace

# Build the Catkin workspace
catkin_make

# Source the setup script for the Catkin workspace
source devel/setup.bash
