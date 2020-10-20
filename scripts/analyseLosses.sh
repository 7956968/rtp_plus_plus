#!/bin/bash

log_file=$1
# Determine base sequence number
# I1015 10:15:26.977782  1462 MemberEntry.cpp:64] [0xb5e00948] Start SN: 47031 Max: 47030 Probation: 2
echo "Analysing "$log_file

mkdir -p losses
loss_file=losses/$log_file"_losses.txt"
echo "Loss file: $loss_file"

sn=$(grep -a "MemberEntry.cpp:[0-9]*] \[0x[0-9,a-f]\{7,8\}] Start SN: " $log_file)
#sn=$(grep -a "MemberEntry.cpp:[0-9]*] \[0x[0-9,a-f]\{7-8\}] Start SN: " $log_file)

if [ -z "$sn" ]; then
  echo "Failed to find start SN line"
  exit 1
fi

set $sn
address=$5
echo "Sequence number base: "$8" Address: "$address >> $loss_file
start_sn=$8

if [ -z "$start_sn" ]; then
  echo "Failed to find start SN"
  exit 1
fi

let last_sn=$start_sn-1
#addr=${address:1:10}
addr=${address:1:${#address}-2}
echo "Address: $addr"
# Read all losses and map to time point
# I1015 12:03:29.856195  1462 MemberEntry.cpp:318] RTP sequence number losses: 12302 12303

losses=$(grep -a "MemberEntry.cpp:[0-9]*] RTP sequence number losses: " $log_file)
#echo "Losses: "$losses

runs_losses=()
runs_good=()
loss_count=0

OLD_IFS=$IFS
IFS=$'\n'
for line in $losses
do
  IFS=$OLD_IFS
  echo "Line: "$line
  set $line
  echo "Count: "$#
  numargs=$#
  # shift out text
  for ((i=1 ; i <= 8 ; i++))
  do
#      echo "Junk: $1"
      shift
  done
  for ((i=1 ; i <= $numargs-8 ; i++))
  do
      cur_sn=$1
      echo "SN: $cur_sn"
      let next_sn=last_sn+1
      if [ $next_sn -eq $1 ]; then
# consecutive loss
        let loss_count=$loss_count+1
      else
# non consecutive loss: store good run
        if [ $cur_sn -lt $last_sn ]; then
          let cur_sn=$cur_sn+65536
        fi
        let good_run=$cur_sn-$last_sn
        runs_good[${#runs_good[@]}]=$good_run
#store previous loss count if there was one
        if [ $loss_count -ne $"0" ]; then
          echo "Storing previous loss run of $loss_count"
          runs_losses[${#runs_losses[@]}]=$loss_count
        fi
        let loss_count=1
      fi

      last_sn=$1    
      shift
  done


  IFS=$'\n'
done

IFS=$OLD_IFS
# Determine last sequence number: note that there are multiple entries of this line
# we need to match the SN with the matching start SN
# I1015 12:03:49.858165  1462 MemberEntry.cpp:50] [0xb5e00948] Destructor: base: 47032 cycles: 196608 max: 12828 received: 162125 lost: 280
last_seq=$(grep -a "MemberEntry.cpp:[0-9]*] \["$addr"] Destructor: base: " $log_file)
echo $last_seq
if [ -z "$last_seq" ]; then
  echo "Failed to find line with final SN, addr was $addr"
  exit 1
fi

set $last_seq

final_sn=${12}
echo "Max SN: "$final_sn
if [ -z "$final_sn" ]; then
  echo "Failed to find final SN"
  exit 1
fi


#grep -a "MemberEntry.cpp:[0-9]*] \[0xb5e00948] Destructor: base: " $log_file

# Store final good run

if [ $final_sn -lt $last_sn ]; then
  let final_sn=$final_sn+65536
fi

# add 'final' good run 
let good_run=$final_sn-$last_sn
runs_good[${#runs_good[@]}]=$good_run

# add 'final' losses
if [ $loss_count -gt 0 ]; then
  runs_losses[${#runs_losses[@]}]=$loss_count
fi

echo "Results:" >> $loss_file

echo "Good runs: "${runs_good[@]} >> $loss_file
echo "Bad runs: "${runs_losses[@]} >> $loss_file

total_packets=0
total_received=0
total_losses=0

for ((i=0; i < ${#runs_good[@]}; i++))
do
  echo ${runs_good[$i]}
  let total_packets=$total_packets+${runs_good[$i]}
  let total_received=$total_received+${runs_good[$i]}
done

for ((i=0; i < ${#runs_losses[@]}; i++))
do
  echo ${runs_losses[$i]}
  let total_packets=$total_packets+${runs_losses[$i]}
  let total_losses=$total_losses+${runs_losses[$i]}
done

echo "Total Packets: $total_packets Received: $total_received Lost: $total_losses" >> $loss_file

