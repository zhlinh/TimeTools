#!/bin/sh

# Usage: sudo sh <filename>
# It will remove ptpd2

PTPD_SYNC=ptpd2

killall ${PTPD_SYNC}

# set the system time from the hardware clock
#hwclock --hctosys

echo "Remove ptpd2 software sync successfully."
