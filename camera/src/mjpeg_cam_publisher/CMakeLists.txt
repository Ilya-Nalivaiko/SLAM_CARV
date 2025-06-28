cmake_minimum_required(VERSION 2.8.3)
project(mjpeg_cam_publisher)

find_package(catkin REQUIRED COMPONENTS
  rospy
  std_msgs
  sensor_msgs
  cv_bridge
)

catkin_package()

catkin_install_python(PROGRAMS
  scripts/mjpeg_camera_node.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
