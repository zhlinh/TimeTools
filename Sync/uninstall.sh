#!/bin/sh

# Usage: sudo sh [filename] [quantity of cores]
# It will sync using guest_sync.c with Hypercall

GUEST_HC_SYNC=guest_sync
HOST_HC_SYNC=host_sync

killall ${GUEST_HC_SYNC}
killall ${HOST_HC_SYNC}

# set the system time from the hardware clock
hwclock --hctosys

echo "Remove sync successfully."
