cmake_minimum_required(VERSION 3.12)
project(ORB_SLAM2_Pub)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

# Common flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__STDC_CONSTANT_MACROS -DGL_GLEXT_PROTOTYPES -D_LINUX -D_REENTRANT -DCGAL_USE_F2C -DCGAL_DISABLE_ROUNDING_MATH_CHECK")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS -DGL_GLEXT_PROTOTYPES -D_LINUX -D_REENTRANT -DCGAL_USE_F2C -DCGAL_DISABLE_ROUNDING_MATH_CHECK")

# suppress all warnings for cleaner build
add_definitions(-w)
add_definitions(-Wno-deprecated)

# Force C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Forcing compilation with C++17 standard.")

# Dependencies
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)
find_package(OpenCV 3.4 REQUIRED)
find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)
find_package(CGAL REQUIRED COMPONENTS Core)
include(${CGAL_USE_FILE})
find_package(Boost REQUIRED)
find_package(LAPACK REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZMQ QUIET zmq)
find_path(ZMQ_INCLUDE_DIR zmq.h PATHS ${PC_ZMQ_INCLUDE_DIRS})
find_library(ZMQ_LIB zmq PATHS ${PC_ZMQ_LIBRARY_DIRS})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/NetworkIntegration
    ${EIGEN3_INCLUDE_DIR}
    ${Pangolin_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/Thirdparty/Line3Dpp
    ${PROJECT_SOURCE_DIR}/Thirdparty/Line3Dpp/build
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${LAPACK_LIBRARY_DIR}
)

# Ensure consistent .so output location
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# ========== Build NetworkIntegrationLib SHARED ==========
add_library(NetworkIntegrationLib SHARED
    src/NetworkIntegration/ChunkCache.cpp
    src/NetworkIntegration/Encoder.cpp
    src/NetworkIntegration/HttpService.cpp
    src/NetworkIntegration/Notifier.cpp
)

SET_target_properties(NetworkIntegrationLib PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(NetworkIntegrationLib
    ${ZMQ_LIB}
)

# ========== Build ORB_SLAM2_Pub SHARED ==========
add_library(${PROJECT_NAME} SHARED
    src/System.cc
    src/Tracking.cc
    src/LocalMapping.cc
    src/LoopClosing.cc
    src/ORBextractor.cc
    src/ORBmatcher.cc
    src/FrameDrawer.cc
    src/Converter.cc
    src/MapPoint.cc
    src/KeyFrame.cc
    src/Map.cc
    src/MapDrawer.cc
    src/Optimizer.cc
    src/PnPsolver.cc
    src/Frame.cc
    src/KeyFrameDatabase.cc
    src/Sim3Solver.cc
    src/Initializer.cc
    src/Viewer.cc
    src/Modeler/FreespaceDelaunayAlgorithm.cc
    src/Modeler/GraphWrapper_Boost.cc
    src/Modeler/lovimath.cc
    src/Modeler/SFMTranscript.cpp
    src/Modeler/SFMTranscriptInterface_Delaunay.cpp
    src/Modeler/Matrix.cc
    src/Modeler/StringFunctions.cpp
    src/Modeler/Exception.cpp
    src/Modeler/SFMTranscriptInterface_ORBSLAM.cpp
    src/Modeler/Modeler.cc
    src/Modeler/ModelDrawer.cc
    src/Modeler/ModelSender.cpp
    src/Modeler/TextureFrame.cc
)

SET_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(${PROJECT_NAME}
    NetworkIntegrationLib
    ${OpenCV_LIBS}
    ${Pangolin_LIBRARIES}
    ${PROJECT_SOURCE_DIR}/Thirdparty/DBoW2/lib/libDBoW2.so
    ${PROJECT_SOURCE_DIR}/Thirdparty/g2o/lib/libg2o.so
    ${PROJECT_SOURCE_DIR}/Thirdparty/EDLines/EDLinesLib.a
    lapack blas
)

# ========== Build tools ==========
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/tools)

add_executable(bin_vocabulary tools/bin_vocabulary.cc)
target_link_libraries(bin_vocabulary ${PROJECT_NAME})
