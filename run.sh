#!/usr/bin/env bash
# Stable TSan runtime wrapper for the ROS node.
# Disables ASLR for this run and forces software GL to avoid VA collisions.

set -euo pipefail

# Good stacks + deadlock detector + keep noise manageable
export ASAN_SYMBOLIZER_PATH="${ASAN_SYMBOLIZER_PATH:-$(command -v llvm-symbolizer || true)}"
export TSAN_OPTIONS="${TSAN_OPTIONS:-detect_deadlocks=1:halt_on_error=1:history_size=7:verbosity=1:log_path=stderr:ignore_noninstrumented_modules=1}"

# Avoid proprietary/OpenGL driver VA reservations clashing with TSan shadow.
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
export QT_XCB_GL_INTEGRATION=none

# Disable ASLR for this invocation only to give TSan predictable address space.
exec setarch "$(uname -m)" -R \
  rosrun ORB_CARV_Pub Mono \
    Vocabulary/ORBvoc.txt \
    config_files/Logitech_c270_HD720p.yaml \
    192.168.1.133 8080 \
    192.168.1.133 5555 \
    camera/image_raw
