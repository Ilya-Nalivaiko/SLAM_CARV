cmake_minimum_required(VERSION 2.8)
project(ORB_SLAM2_Pub)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall  -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall   -O3")

# set carv needed flags
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

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

find_package(OpenCV 3.4 REQUIRED)
find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)

# Find CGAL
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE "Release")
find_package(CGAL REQUIRED COMPONENTS Core)
include( ${CGAL_USE_FILE} )
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
# Find Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
# BLAS and LAPACK
find_package(LAPACK REQUIRED)
link_directories(${LAPACK_LIBRARY_DIR})


include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/NetworkIntegration
        ${PROJECT_SOURCE_DIR}/include/NetworkIntegration/external
        ${EIGEN3_INCLUDE_DIR}
        ${Pangolin_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/Thirdparty/Line3Dpp
        ${PROJECT_SOURCE_DIR}/Thirdparty/Line3Dpp/build
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_VISIBILITY_INLINES_HIDDEN OFF)

# added carv source file
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
        src/NetworkIntegration/ChunkCache.cpp
        src/NetworkIntegration/Encoder.cpp
        src/NetworkIntegration/HttpService.cpp
        src/NetworkIntegration/Notifier.cpp
        )

# ZMQ Library
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZMQ QUIET zmq)
find_path(ZMQ_INCLUDE_DIR zmq.h PATHS ${PC_ZMQ_INCLUDE_DIRS})
find_library(ZMQ_LIB zmq PATHS ${PC_ZMQ_LIBRARY_DIRS})
mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_LIB)
include_directories(${ZMQ_INCLUDE_DIR})
set(LIBS ${LIBS} ${ZMQ_LIB})

# add lapack blas link
target_link_libraries(${PROJECT_NAME}
        ${OpenCV_LIBS}
        ${EIGEN3_LIBS}
        ${Pangolin_LIBRARIES}
        ${PROJECT_SOURCE_DIR}/Thirdparty/DBoW2/lib/libDBoW2.so
        ${PROJECT_SOURCE_DIR}/Thirdparty/g2o/lib/libg2o.so
        ${PROJECT_SOURCE_DIR}/Thirdparty/EDLines/EDLinesLib.a
        lapack blas
        )

# Build examples

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/RGB-D)

# add_executable(rgbd_tum
#         Examples/RGB-D/rgbd_tum.cc)
# target_link_libraries(rgbd_tum ${PROJECT_NAME})

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Stereo)

# add_executable(stereo_kitti
#         Examples/Stereo/stereo_kitti.cc)
# target_link_libraries(stereo_kitti ${PROJECT_NAME})

# add_executable(stereo_euroc
#         Examples/Stereo/stereo_euroc.cc)
# target_link_libraries(stereo_euroc ${PROJECT_NAME})


# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Monocular)

# add_executable(mono_tum
#         Examples/Monocular/mono_tum.cc)
# target_link_libraries(mono_tum ${PROJECT_NAME})

# add_executable(mono_kitti
#         Examples/Monocular/mono_kitti.cc)
# target_link_libraries(mono_kitti ${PROJECT_NAME})

# add_executable(mono_euroc
#         Examples/Monocular/mono_euroc.cc)
# target_link_libraries(mono_euroc ${PROJECT_NAME})

# add_executable(mono_mono
#         Examples/Monocular/mono_mono.cc)
# target_link_libraries(mono_mono ${PROJECT_NAME})

# Build tools
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/tools)
add_executable(bin_vocabulary
        tools/bin_vocabulary.cc)
target_link_libraries(bin_vocabulary ${PROJECT_NAME})

