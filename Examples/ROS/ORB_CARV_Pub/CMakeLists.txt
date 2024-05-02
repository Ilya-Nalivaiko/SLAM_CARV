cmake_minimum_required(VERSION 2.8.3)
project(ORB_SLAM2_Pub)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall  -O3 -march=native ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -O3 -march=native")

# Check for C++14 support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("Using flag -std=c++14.")

# Find packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  cv_bridge
)

# Include directories
include_directories(
  ${catkin_INCLUDE_DIRS}
)

# Add executable for monocular camera
add_executable(Mono
  src/ros_mono.cc
)

target_link_libraries(Mono
  ${catkin_LIBRARIES}
)

# Note: Nodes for stereo camera and RGB-D camera are commented out
# as they are not currently needed. You can uncomment them if needed.

#comment those nodes
if(FALSE) # fake a block comment
# Node for monocular camera (Augmented Reality Demo)
rosbuild_add_executable(MonoAR
src/AR/ros_mono_ar.cc
src/AR/ViewerAR.h
src/AR/ViewerAR.cc
)

target_link_libraries(MonoAR
${LIBS}
)

# Node for stereo camera
rosbuild_add_executable(Stereo
src/ros_stereo.cc
)

target_link_libraries(Stereo
${LIBS}
)

# Node for RGB-D camera
rosbuild_add_executable(RGBD
src/ros_rgbd.cc
)

target_link_libraries(RGBD
${LIBS}
)

endif()
