#!/usr/bin/env bash
# Always build the ROS node with ThreadSanitizer enabled.
# Robust to both Clang and GCC; prefers Clang's TSan runtime.

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
  # Fallback to GCC (TSan works but is generally less robust than Clang's)
  CC_BIN="$(command -v gcc || true)"
  CXX_BIN="$(command -v g++)"
  if [[ -z "${CC_BIN}" || -z "${CXX_BIN}" ]]; then
    echo "ERROR: No suitable compiler found (clang/clang++ or gcc/g++)." >&2
    exit 1
  fi
  TSAN_SO="$("$CC_BIN" -print-file-name=libtsan.so)"
fi

if [[ ! -f "$TSAN_SO" ]]; then
  echo "WARNING: Could not locate TSan runtime via compiler; continuing without explicit RPATH." >&2
  TSAN_RT_DIR=""
else
  TSAN_RT_DIR="$(dirname "$TSAN_SO")"
fi

echo "[build_ros] Using CC=${CC_BIN}"
echo "[build_ros] Using CXX=${CXX_BIN}"
[[ -n "${TSAN_RT_DIR}" ]] && echo "[build_ros] TSan runtime dir: ${TSAN_RT_DIR}"

# --- Project paths ---
# Run from repo root regardless of caller location.
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}"
ROS_PKG_DIR="${REPO_ROOT}/Examples/ROS/ORB_CARV_Pub"
BUILD_DIR="${ROS_PKG_DIR}/build"

if [[ ! -d "${ROS_PKG_DIR}" ]]; then
  echo "ERROR: Expected ROS package at ${ROS_PKG_DIR} not found." >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# --- Common sanitizer/PIE flags ---
# -fPIC everywhere, executables get -fPIE/-pie. Keep frame pointers.
C_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -fPIC -g -O1 -pthread"
CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -fPIC -fPIE -g -O1 -pthread"
EXE_LINK_FLAGS="-fsanitize=thread -pie -pthread"
SHARED_LINK_FLAGS="-fsanitize=thread -pthread"

# Add RPATH to prefer the chosen TSan runtime (helps avoid mixing runtimes).
if [[ -n "${TSAN_RT_DIR}" ]]; then
  EXE_LINK_FLAGS="${EXE_LINK_FLAGS} -Wl,-rpath,${TSAN_RT_DIR}"
  SHARED_LINK_FLAGS="${SHARED_LINK_FLAGS} -Wl,-rpath,${TSAN_RT_DIR}"
fi

# Avoid eager binding of all symbols which can pull in stuff before TSan init.
EXE_LINK_FLAGS="${EXE_LINK_FLAGS} -Wl,-z,relro"
SHARED_LINK_FLAGS="${SHARED_LINK_FLAGS} -Wl,-z,relro"

echo "[build_ros] Configuring CMake in ${BUILD_DIR} ..."
cmake .. \
  -DCMAKE_C_COMPILER="${CC_BIN}" \
  -DCMAKE_CXX_COMPILER="${CXX_BIN}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="${C_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${CXX_FLAGS}" \
  -DCMAKE_EXE_LINKER_FLAGS="${EXE_LINK_FLAGS}" \
  -DCMAKE_SHARED_LINKER_FLAGS="${SHARED_LINK_FLAGS}"

echo "[build_ros] Building ..."
make -j"$(nproc)"

echo "[build_ros] Done."
