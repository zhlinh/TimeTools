#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using host_sync.c with IOCTL

HOST_IO_SYNC=host_sync

gcc ${HOST_IO_SYNC}.c -o  ${HOST_IO_SYNC} -lrt
./${HOST_IO_SYNC} -d
