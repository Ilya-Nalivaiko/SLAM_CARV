#!/usr/bin/env bash
# Build the ROS node with TSan **iff** USE_TSAN is set; otherwise use Release flags.
# Robust to both Clang and GCC; prefers Clang's TSan runtime when available.

set -euo pipefail

# --- Choose compiler toolchain (prefer Clang's TSan) ---
if command -v clang-18 >/dev/null 2>&1; then
  CC_BIN="$(command -v clang-18)"
  CXX_BIN="$(command -v clang++-18)"
  TSAN_SO="$("$CC_BIN" -print-file-name=libclang_rt.tsan-x86_64.so)"
elif command -v clang >/dev/null 2>&1; then
  CC_BIN="$(command -v clang)"
  CXX_BIN="$(command -v clang++)"
  TSAN_SO="$("$CC_BIN" -print-file-name=libclang_rt.tsan-x86_64.so)"
else
  # Fallback to GCC toolchain
  if command -v gcc-10 >/dev/null 2>&1; then
    CC_BIN="$(command -v gcc-10)"
    CXX_BIN="$(command -v g++-10)"
  else
    CC_BIN="$(command -v gcc)"
    CXX_BIN="$(command -v g++)"
  fi
  TSAN_SO="" # GCC TSan runtime is linked via -fsanitize=thread, rpath optional
fi

if [[ -n "${TSAN_SO:-}" && -z "${TSAN_RT_DIR:-}" ]]; then
  TSAN_RT_DIR="$(dirname "${TSAN_SO}")"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROS_PKG_DIR="${SCRIPT_DIR}/Examples/ROS/ORB_CARV_Pub"
BUILD_DIR="${ROS_PKG_DIR}/build"

if [[ ! -d "${ROS_PKG_DIR}" ]]; then
  echo "ERROR: Expected ROS package at ${ROS_PKG_DIR} not found." >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [[ -n "${USE_TSAN:-}" ]]; then
  echo " ==== BUILDING WITH TSAN ==== "
  # --- Common sanitizer/PIE flags ---
  # -fPIC everywhere, executables get -fPIE/-pie. Keep frame pointers.
  BUILD_TYPE=Debug
  C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -fPIC -g -O1 -pthread"
  CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -fPIC -fPIE -g -O1 -pthread"
  EXE_LINK_FLAGS="-fsanitize=thread -pie -pthread"
  SHARED_LINK_FLAGS="-fsanitize=thread -pthread"

  # Add RPATH to prefer the chosen TSan runtime (helps avoid mixing runtimes).
  if [[ -n "${TSAN_RT_DIR:-}" ]]; then
    EXE_LINK_FLAGS="${EXE_LINK_FLAGS} -Wl,-rpath,${TSAN_RT_DIR}"
    SHARED_LINK_FLAGS="${SHARED_LINK_FLAGS} -Wl,-rpath,${TSAN_RT_DIR}"
  fi

  # Avoid eager binding of all symbols which can pull in stuff before TSan init.
  EXE_LINK_FLAGS="${EXE_LINK_FLAGS} -Wl,-z,relro"
  SHARED_LINK_FLAGS="${SHARED_LINK_FLAGS} -Wl,-z,relro"
else
  echo " ==== BUILDING RELEASE ==== "
  # --- Default release flags ---
  BUILD_TYPE=Release
  C_FLAGS="-O2 -DNDEBUG -fPIC"
  CXX_FLAGS="-O2 -DNDEBUG -fPIC"
  EXE_LINK_FLAGS=""
  SHARED_LINK_FLAGS=""
fi

echo "[build_ros] Configuring CMake in ${BUILD_DIR} ..."
cmake .. \
  -DCMAKE_C_COMPILER="${CC_BIN}" \
  -DCMAKE_CXX_COMPILER="${CXX_BIN}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_C_FLAGS="${C_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${CXX_FLAGS}" \
  -DCMAKE_EXE_LINKER_FLAGS="${EXE_LINK_FLAGS}" \
  -DCMAKE_SHARED_LINKER_FLAGS="${SHARED_LINK_FLAGS}"

echo "[build_ros] Building ..."
make -j"$(nproc)"

echo "[build_ros] Done."
