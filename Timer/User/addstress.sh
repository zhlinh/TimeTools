#!/bin/sh

# Usage: sudo sh [filename] [quantity of cores]
# You can use 'top' and then press 1 to see the stress on cpus.

LOAD=load
n=1

if [ $1 ]; then
	n=$1
else
	echo "Install failed.You didn't assign the quantity of cores."
	echo "Usage: sudo sh $0 [quantity of cores]"
  echo "--Example: sudo sh $0 2\n"
  exit
fi

gcc ${LOAD}.c -o  ${LOAD} -lrt

echo "quantity of cores is $n"
while [ $n -gt 0 ]
do
  n=`expr $n - 1`
	taskset -c $n ./${LOAD} &
done
