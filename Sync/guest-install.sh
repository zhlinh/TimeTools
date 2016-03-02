#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using guest_sync.c with Hypercall

GUEST_HC_SYNC=guest_sync

gcc ${GUEST_HC_SYNC}.c -o  ${GUEST_HC_SYNC} -lrt
./${GUEST_HC_SYNC} -d
