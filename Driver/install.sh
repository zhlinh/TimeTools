#!/bin/sh
#by ben

module="pcie_host_device"
device="pcietime0"
mode="664"

make

rm -f /dev/${device}
#rmmod $module
insmod ./$module.ko

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

echo mknod $module
echo device major is $major
#echo device name is $device

mknod /dev/${device} c $major 0
chmod $mode /dev/${device}

# if you don't need sync, you can comment the following two lines.
gcc time_sync.c -o sync -lrt
./sync -d
