cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

rosbuild_init()

IF(NOT ROS_BUILD_TYPE)
  SET(ROS_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${ROS_BUILD_TYPE})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++17")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")

# ===========================
# Force C++17
# ===========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Forcing compilation with C++17 standard.")

# ===========================
# Modules & Dependencies
# ===========================
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake_modules)

find_package(OpenCV 3.4 REQUIRED)
find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)

# CGAL
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE "Release")
find_package(CGAL REQUIRED COMPONENTS Core)
include(${CGAL_USE_FILE})
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})

# Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

# LAPACK and BLAS
find_package(LAPACK REQUIRED)
link_directories(${LAPACK_LIBRARY_DIR})

# ===========================
# Include directories
# ===========================
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/../../../
  ${PROJECT_SOURCE_DIR}/../../../include
  ${PROJECT_SOURCE_DIR}/../../../include/NetworkIntegration
  ${PROJECT_SOURCE_DIR}/../../../include/NetworkIntegration/external        # <---- Added for httplib.h
  ${Pangolin_INCLUDE_DIRS}
)

# ===========================
# Libraries
# ===========================
set(LIBS
  ${OpenCV_LIBS}
  ${EIGEN3_LIBS}
  ${Pangolin_LIBRARIES}
  ${PROJECT_SOURCE_DIR}/../../../Thirdparty/DBoW2/lib/libDBoW2.so
  ${PROJECT_SOURCE_DIR}/../../../Thirdparty/g2o/lib/libg2o.so
  ${PROJECT_SOURCE_DIR}/../../../lib/libORB_SLAM2_Pub.so
  /usr/local/lib/libopencv_core.so.3.4
  /usr/local/lib/libopencv_imgproc.so.3.4
  -lboost_system
  lapack
  blas
)

# ===========================
# Executables and linking
# ===========================
# Node for monocular camera
rosbuild_add_executable(Mono
  src/ros_mono.cc
)

target_link_libraries(Mono
  ${LIBS}
)

# Explicitly enforce C++17 for Mono (some older rosbuild may ignore global settings)
if(COMMAND target_compile_features)
  target_compile_features(Mono PUBLIC cxx_std_17)
endif()

# ========== Disabled nodes ==========
if(FALSE) # fake block comment

# Node for monocular camera (AR)
rosbuild_add_executable(MonoAR
  src/AR/ros_mono_ar.cc
  src/AR/ViewerAR.h
  src/AR/ViewerAR.cc
)
target_link_libraries(MonoAR ${LIBS})

# Node for stereo camera
rosbuild_add_executable(Stereo
  src/ros_stereo.cc
)
target_link_libraries(Stereo ${LIBS})

# Node for RGB-D camera
rosbuild_add_executable(RGBD
  src/ros_rgbd.cc
)
target_link_libraries(RGBD ${LIBS})

endif()
