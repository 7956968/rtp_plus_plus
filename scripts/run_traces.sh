#!/bin/bash
cd ~/rtp++
mkdir -p log/traces
hosts=$(cat 195.148.127.98_hosts.txt)

echo $hosts
dt=`eval date +%Y%m%d"_"%H_%M_%S`

for host in $hosts
do
 sh tr.sh $host $dt 
done

