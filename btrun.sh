LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so \
SEGFAULT_SIGNALS=abrt \
LIBC_FATAL_STDERR_=1 \
./run.sh 2> errlog.txt