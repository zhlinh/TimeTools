#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using guest_sync.c with Hypercall

GUEST_HC_SYNC=guest_sync

gcc ${GUEST_HC_SYNC}.c -o  ${GUEST_HC_SYNC} -lrt

if [ $1 ]; then
    ./${GUEST_HC_SYNC} -d -s $1
    echo "If you also want to set sync interval (Commonly no need to set):\n\tsudo sh $0 [sync interval(default 10000(means 10ms))]\n"    
else
    ./${GUEST_HC_SYNC} -d
    echo "If you also want to set sync interval (Commonly no need to set):\n\tsudo sh $0 [sync interval(default 10000(means 10ms))]\n"
fi

