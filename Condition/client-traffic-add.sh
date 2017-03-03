#!/bin/sh

# Usage: sudo sh <filename> <server address> <network traffic(b/s)> <running time(s)> <using bidirection(true/false)>
# Install iper3 to add traffic: sudo apt-get install iperf3

OUTER_APP=iperf3
SEVER_ADDR=192.168.1.18
TRAFFIC_b_s=100M
RUNNING_TIME_s=10
BI_DIRECTION=false

PREFIX=`date "+%Y-%m-%d_%S"`

if [ $1 ] && [ $2 ] && [ $3 ] && [ $4 ]; then
    SEVER_ADDR=$1
    TRAFFIC_b_s=$2
    RUNNING_TIME_s=$3
    BI_DIRECTION=$4
else
    echo "Execute failed. You didn't assign server address, network traffic(b/s), running time(s) and whether using bidirection transmition."
    echo "Usage: sudo sh $0 <server address> <network traffic(b/s)> <running time(s)> <using bidirection(true/false)>"
    echo "--Example: sudo sh $0 192.168.1.18 100M 10 false\n"
    exit
fi

echo "server address is       ${SEVER_ADDR}"
echo "network traffic(b/s) is ${TRAFFIC_b_s} b/s"
echo "running time(s) is      ${RUNNING_TIME_s} s"
echo "bidirection is          ${BI_DIRECTION}"
echo "iperf3 client is sending data..."

# use UDP mode
if ${BI_DIRECTION}; then
    ${OUTER_APP} -c ${SEVER_ADDR} -u -b ${TRAFFIC_b_s} -t ${RUNNING_TIME_s} -d > "${PREFIX}_traffic-client.log"
else
    ${OUTER_APP} -c ${SEVER_ADDR} -u -b ${TRAFFIC_b_s} -t ${RUNNING_TIME_s} > "${PREFIX}_traffic-client.log"
    exit
fi

