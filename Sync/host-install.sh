#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using host_sync.c with IOCTL

HOST_IO_SYNC=host_sync

gcc ${HOST_IO_SYNC}.c -o  ${HOST_IO_SYNC} -lrt

if [ $1 ]; then
    ./${HOST_IO_SYNC} -d -s $1
else
    ./${HOST_IO_SYNC} -d
fi
