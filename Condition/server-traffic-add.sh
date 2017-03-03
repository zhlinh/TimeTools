#!/bin/sh

# Usage: sudo sh <filename> <using bidirection(true/false)>
# Install iper3 to add traffic: sudo apt-get install iperf3

OUTER_APP=iperf3
BI_DIRECTION=false

if [ $1 ]; then
    BI_DIRECTION=$1
else
    echo "Execute failed. You didn't assign whether using bidirection transmition."
    echo "Usage: sudo sh $0 <using bidirection(true/false)>"
    echo "--Example: sudo sh $0 false\n"
    exit
fi

echo "bidirection is  ${BI_DIRECTION}"
echo "iperf3 server is listening..."

# use UDP mode
if ${BI_DIRECTION}; then
    ${OUTER_APP} -s -d > "traffic-server.log"
else
    ${OUTER_APP} -s > "traffic-server.log"
    exit
fi
