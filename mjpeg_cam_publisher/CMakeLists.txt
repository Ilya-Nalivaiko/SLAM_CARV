cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
rosbuild_init(mjpeg_cam_publisher)
# No rosbuild_add_executable here — Python doesn’t compile
