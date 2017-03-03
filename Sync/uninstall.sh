#!/bin/sh

# Usage: sudo sh <filename>
# It will remove guest_sync or host_sync

GUEST_HC_SYNC=guest_sync
HOST_HC_SYNC=host_sync

killall ${GUEST_HC_SYNC}
killall ${HOST_HC_SYNC}

# set the system time from the hardware clock
hwclock --hctosys

echo "Remove ptp card sync successfully."

