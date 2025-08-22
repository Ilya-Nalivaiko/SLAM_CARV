#!/usr/bin/env bash
echo "Building ROS nodes"

cd Examples/ROS/ORB_CARV_Pub
mkdir build
cd build

cmake .. \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-10 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-10 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g -pthread" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -pthread" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread -pthread"

make clean
make -j 2
