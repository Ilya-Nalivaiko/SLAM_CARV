#!/usr/bin/env bash
# Collects GDB thread backtraces and a bounded strace snapshot for a target process.
# Default target is the ROS node "Mono".
#

set -euo pipefail

PROC_NAME="Mono"
PID=""
DURATION=5            # seconds for time-limited strace
LINE_LIMIT=30           # 0 = disabled; else limit strace lines
OUTDIR_BASE="./tsan_dumps"

# --- locate PID(s) ---
if [[ -z "$PID" ]]; then
  # Exact name first; fallback to pidof; final fallback to pgrep -f (substring).
  if command -v pgrep >/dev/null 2>&1; then
    mapfile -t PIDS < <(pgrep -x "$PROC_NAME" || true)
    if [[ ${#PIDS[@]} -eq 0 ]]; then
      mapfile -t PIDS < <(pgrep -f "$PROC_NAME" || true)
    fi
  else
    PIDS=($(pidof "$PROC_NAME" 2>/dev/null || true))
  fi
else
  PIDS=("$PID")
fi

if [[ ${#PIDS[@]} -eq 0 ]]; then
  echo "ERROR: No running process found for name='$PROC_NAME' (and no --pid given)." >&2
  exit 1
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUTDIR="${OUTDIR_BASE}/${PROC_NAME}_${TS}"
mkdir -p "$OUTDIR"

echo "[collect_debug] Target PIDs: ${PIDS[*]}"
echo "[collect_debug] Output dir : ${OUTDIR}"
echo "[collect_debug] Mode       : $([[ $LINE_LIMIT -gt 0 ]] && echo "strace line-limit=$LINE_LIMIT" || echo "strace duration=${DURATION}s")"

# Helper: safe run to avoid set -e on timeout/non-zero
_run() {
  echo "[collect_debug] $*"
  bash -c "$*" || true
}

# Dump a bunch of useful /proc info
_dump_proc() {
  local pid="$1"
  cp "/proc/$pid/maps"        "$OUTDIR/proc.${pid}.maps"        2>/dev/null || true
  cp "/proc/$pid/status"      "$OUTDIR/proc.${pid}.status"      2>/dev/null || true
  cp "/proc/$pid/sched"       "$OUTDIR/proc.${pid}.sched"       2>/dev/null || true
  cp "/proc/$pid/stack"       "$OUTDIR/proc.${pid}.kstack"      2>/dev/null || true
  ls -l "/proc/$pid/fd"     > "$OUTDIR/proc.${pid}.fds.txt"     2>/dev/null || true
  for t in /proc/$pid/task/*; do
    [[ -d "$t" ]] || continue
    tid="$(basename "$t")"
    cat "$t/stack"  > "$OUTDIR/proc.${pid}.task.${tid}.kstack"  2>/dev/null || true
    cat "$t/wchan"  > "$OUTDIR/proc.${pid}.task.${tid}.wchan"   2>/dev/null || true
    cat "$t/stat"   > "$OUTDIR/proc.${pid}.task.${tid}.stat"    2>/dev/null || true
  done
  lsof -p "$pid" > "$OUTDIR/proc.${pid}.lsof.txt" 2>/dev/null || true
  ps -Lp "$pid" -o pid,tid,pcpu,psr,stat,wchan:32,comm > "$OUTDIR/proc.${pid}.threads.ps" 2>&1 || true
}

# GDB snapshot (one-shot, all threads, full backtraces)
_gdb_dump() {
  local pid="$1"
  local outf="$OUTDIR/gdb.bt.${pid}.txt"
  if ! command -v gdb >/dev/null 2>&1; then
    echo "[collect_debug] gdb not found; skipping GDB dump for PID $pid" | tee -a "$OUTF" 2>/dev/null || true
    return
  fi
  echo "[collect_debug] Capturing GDB backtrace for PID $pid -> $outf"
  gdb -batch -q -p "$pid" \
    -ex "set pagination off" \
    -ex "thread apply all bt full" \
    -ex "info threads" \
    -ex "set print pretty on" \
    -ex "set logging file $outf.mutexes" \
    -ex "set logging on" \
    -ex "python import gdb, re
    frames = gdb.execute('thread apply all bt', to_string=True)
    for line in frames.splitlines():
        m = re.search(r'pthread_mutex_lock.*0x([0-9a-f]+)', line)
        if m:
            addr = m.group(1)
            try:
                gdb.execute('p *(pthread_mutex_t*)0x%s' % addr)
            except:
                pass
    " \
    -ex "set logging off" \
    -ex "quit" > "$outf" 2>&1 || true
}

# Strace snapshot
_strace_time() {
  local pid="$1"
  # -ff splits by pid into multiple files, base name below
  local base="$OUTDIR/strace.${pid}"
  echo "[collect_debug] Running time-limited strace (${DURATION}s) for PID $pid -> ${base}.*"
  timeout "${DURATION}"s strace -f -e trace=futex,epoll_wait,epoll_pwait,recvfrom,sendto,nanosleep -tt -T -yy -s 256 -ff -o "$base" -p "$pid" 2>/dev/null || true
}

_strace_lines() {
  local pid="$1"
  local outf="$OUTDIR/strace.${pid}.lines.log"
  echo "[collect_debug] Running line-limited strace (first ${LINE_LIMIT} lines) for PID $pid -> $outf"
  # strace logs to stderr; capture and cut after N lines
  # stdbuf helps line-buffer the pipe
  strace -f -e trace=futex,epoll_wait,epoll_pwait,recvfrom,sendto,nanosleep -tt -T -yy -s 256 -p "$pid" 2> >(stdbuf -oL -eL head -n "$LINE_LIMIT" > "$outf") 1>/dev/null || true
}

# System context
{
  echo "==== collect_debug context ===="
  date -Is
  uname -a
  command -v gdb   && gdb --version   | head -n1
  command -v strace && strace -V      | head -n1
  echo "PROC_NAME=$PROC_NAME PID_ARG=${PID:-} PIDS=${PIDS[*]}"
  echo "DURATION=$DURATION LINE_LIMIT=$LINE_LIMIT"
} > "$OUTDIR/_context.txt" 2>&1

# Run for each PID
for p in "${PIDS[@]}"; do
  _dump_proc "$p"
  _gdb_dump "$p"
  if [[ "$LINE_LIMIT" -gt 0 ]]; then
    _strace_lines "$p"
  else
    _strace_time "$p"
  fi
done

echo "[collect_debug] Done. Files in: $OUTDIR"
