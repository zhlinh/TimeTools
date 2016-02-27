#!/bin/sh

# Usage: sudo sh [filename] [quantity of cores]

LOAD=load
n=1

if [ $1 ]; then
	n=$1
fi

gcc ${LOAD}.c -o  ${LOAD} -lrt

echo "quantity of cores is $n"
while [ $n -gt 0 ]
do
  n=`expr $n - 1`
	taskset -c $n ./${LOAD} &
done
