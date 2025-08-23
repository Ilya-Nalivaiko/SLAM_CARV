#!/usr/bin/env bash
echo "Building ROS nodes"

cd Examples/ROS/ORB_CARV_Pub
mkdir build
cd build

# Runtime search paths for the chosen toolchain (helps avoid libtsan from gcc-9)
GCC_TSAN_DIR="$(dirname "$(gcc-10 -print-file-name=libtsan.so)")"
GCC_STDCXX_DIR="$(dirname "$(g++-10 -print-file-name=libstdc++.so.6)")"
export EXTRA_RPATH="-Wl,-rpath,${GCC_TSAN_DIR} -Wl,-rpath,${GCC_STDCXX_DIR}"

cmake .. \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-10 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-10 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g -pthread" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -pthread $EXTRA_RPATH" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread -pthread $EXTRA_RPATH"

make clean
make -j 2
