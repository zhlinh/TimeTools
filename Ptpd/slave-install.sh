#!/bin/sh

# Usage: sudo sh [filename]
# It will sync using ptpd2 software
# Ptpd2 will work on slave mode.

etc_dir="/etc"
target="ptpd2-slave.conf"
fmode="664"
interface="eth0"

if [ $1 ]; then
	interface=$1
else
	echo "Install failed. You didn't assign the interface."
	echo "Usage: sudo sh $0 [interface]"
  echo "--Example: sudo sh $0 eth0\n"
  exit
fi

chmod $fmode ./${target}
cp -af ./${target} ${etc_dir}/ptpd2.conf

sed -ig "s/interface=eth0/interface=${interface}/" ${etc_dir}/ptpd2.conf

ptpd2 -c /etc/ptpd2.conf
