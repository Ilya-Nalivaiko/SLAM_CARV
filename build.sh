#!/usr/bin/env bash

# ================================
# Forced environment setup
# ================================

# Force GCC 8 explicitly
export CC=/usr/bin/gcc-8
export CXX=/usr/bin/g++-8

echo "[INFO] Using compilers:"
which gcc
which g++
gcc --version
g++ --version

# Pangolin & OpenCV runtime safety
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# ================================
# Build Thirdparty/DBoW2
# ================================
echo "[INFO] Configuring and building Thirdparty/DBoW2 ..."
cd Thirdparty/DBoW2
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../../../

# ================================
# Build Thirdparty/EDLines
# ================================
echo "[INFO] Configuring and building Thirdparty/EDLines ..."
cd Thirdparty/EDLines
make clean
make
cd ../../

# ================================
# Build Thirdparty/g2o
# ================================
echo "[INFO] Configuring and building Thirdparty/g2o ..."
cd Thirdparty/g2o
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../../../

# ================================
# Uncompress Vocabulary
# ================================
echo "[INFO] Uncompressing ORB vocabulary ..."
cd Vocabulary
if [ ! -f ORBvoc.txt ]; then
    tar -xf ORBvoc.txt.tar.gz
else
    echo "[INFO] ORBvoc.txt already extracted."
fi
cd ..

# ================================
# Configure and build ORB_SLAM2_Pub
# ================================
echo "[INFO] Configuring and building ORB_SLAM2_Pub ..."

rm -rf build
mkdir build
cd build

echo "[INFO] Running CMake configuration..."
cmake .. -DCMAKE_BUILD_TYPE=Release | tee ../cmake_configure.log
CMAKE_EXIT_CODE=${PIPESTATUS[0]}
if [ $CMAKE_EXIT_CODE -ne 0 ]; then
    echo "[ERROR] CMake configuration failed. Check cmake_configure.log."
    exit 1
fi

echo "[INFO] Building with make..."
make -j$(nproc) | tee ../build.log
MAKE_EXIT_CODE=${PIPESTATUS[0]}
if [ $MAKE_EXIT_CODE -ne 0 ]; then
    echo "[ERROR] Build failed. Check build.log."
    exit 1
fi
cd ..

# ================================
# Convert vocabulary to binary
# ================================
echo "[INFO] Converting vocabulary to binary using tools/bin_vocabulary ..."
./tools/bin_vocabulary

# ================================
# Inline build flag and environment analysis
# ================================
echo ""
echo "===== Post-Build Analysis ====="

echo ""
echo "[INFO] Checking all compiler invocations..."
grep -E "(/usr/bin/c\+\+|/usr/bin/g\+\+|c\+\+|g\+\+)" build.log | tee compiler_invocations.txt

echo ""
echo "[INFO] Checking for -std=c++17 flags..."
grep -E "(-std=c\+\+17)" build.log | tee cxx17_flags.txt

echo ""
echo "[INFO] C++ standard flag summary:"
grep -oE --color=always "(-std=c\+\+[0-9]+)" build.log | sort | uniq -c

echo ""
echo "[INFO] Checking for OpenCV includes during compilation..."
grep -E "opencv" build.log | grep -E "include|opencv" | tee opencv_includes.txt

echo ""
echo "[INFO] Checking for compiler warnings..."
grep -i "warning" build.log | tee warnings_in_build.txt

echo ""
echo "[INFO] Checking for OpenCV shared libraries on system:"
find /usr/local/lib -name "libopencv_core.so*"
find /usr/lib -name "libopencv_core.so*"

echo ""
echo "[INFO] Checking OpenCV version via pkg-config:"
pkg-config --modversion opencv || echo "pkg-config opencv not found"
pkg-config --modversion opencv4 || echo "pkg-config opencv4 not found"

echo ""
echo "[INFO] Testing OpenCV compile and runtime version..."
echo -e '#include <opencv2/core.hpp>\n#include <iostream>\nint main(){std::cout<<CV_VERSION<<std::endl;return 0;}' > test_opencv.cpp
g++ -std=c++17 test_opencv.cpp -o test_opencv `pkg-config --cflags --libs opencv4` || \
g++ -std=c++17 test_opencv.cpp -o test_opencv `pkg-config --cflags --libs opencv` || \
echo "[ERROR] Failed to compile OpenCV test program"
./test_opencv || echo "[ERROR] OpenCV runtime test failed"

echo ""
echo "[INFO] Testing std::shared_mutex support..."
echo -e '#include <shared_mutex>\nint main(){std::shared_mutex m; return 0;}' > test_mutex.cpp
g++ -std=c++17 test_mutex.cpp -o test_mutex
./test_mutex && echo "[INFO] std::shared_mutex test succeeded." || echo "[ERROR] std::shared_mutex test failed."

echo ""
echo "[INFO] Checking ldd on test_mutex:"
ldd ./test_mutex

echo ""
echo "===== Post-Build Analysis Complete ====="
echo "Generated filtered logs:"
echo "  - compiler_invocations.txt"
echo "  - cxx17_flags.txt"
echo "  - opencv_includes.txt"
echo "  - warnings_in_build.txt"

echo ""
echo "[INFO] Full build and validation completed successfully."
