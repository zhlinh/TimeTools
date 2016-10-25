#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using host_sync.c with IOCTL

HOST_IO_SYNC=host_sync

gcc ${HOST_IO_SYNC}.c -o ${HOST_IO_SYNC} -lrt

if [ $1 ]; then
    ./${HOST_IO_SYNC} -d -s $1
    echo "If you also want to set sync interval (Commonly no need to set):\n\tsudo sh $0 [sync interval(default 10000(means 10ms))]\n"    
else
    ./${HOST_IO_SYNC} -d
    echo "If you also want to set sync interval (Commonly no need to set):\n\tsudo sh $0 [sync interval(default 10000(means 10ms))]\n"    
fi

