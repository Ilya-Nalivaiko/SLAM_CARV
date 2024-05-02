cmake_minimum_required(VERSION 2.8.3)
project(ORB_SLAM2_Pub)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -march=native")

# Check for C++14 support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("Using flag -std=c++14.")

# Set module path
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake_modules)

# Find packages
find_package(OpenCV 3.0 QUIET)
if(NOT OpenCV_FOUND)
  find_package(OpenCV 2.4.3 QUIET)
  if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV > 2.4.3 not found.")
  endif()
endif()

find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)
find_package(CGAL REQUIRED COMPONENTS Core)
find_package(Boost REQUIRED)
find_package(LAPACK REQUIRED)

# Include directories
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/../../../
  ${PROJECT_SOURCE_DIR}/../../../include
  ${Pangolin_INCLUDE_DIRS}
)

# Add executable for monocular camera
add_executable(Mono
  src/ros_mono.cc
)

target_link_libraries(Mono
  ${OpenCV_LIBS} 
  ${EIGEN3_LIBS}
  ${Pangolin_LIBRARIES}
  ${PROJECT_SOURCE_DIR}/../../../Thirdparty/DBoW2/lib/libDBoW2.so
  ${PROJECT_SOURCE_DIR}/../../../Thirdparty/g2o/lib/libg2o.so
  ${PROJECT_SOURCE_DIR}/../../../lib/libORB_SLAM2_Pub.so
  ${LAPACK_LIBRARIES}
  ${BLAS_LIBRARIES}
)

# Note: Nodes for stereo camera and RGB-D camera are commented out
# as they are not currently needed. You can uncomment them if needed.
