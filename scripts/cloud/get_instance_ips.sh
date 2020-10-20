#!/bin/bash

regions=$(cat regions.txt)
OLDIFS=$IFS

IFS=$'\n'

region_ids=()
# iterate over regions file to store the region identifier
for region in $regions
do
  IFS=$OLDIFS
  set $region
  region_id=$5
  echo "Region: $region_id"
  region_ids[${#region_ids[@]}]=$region_id
done

INSTANCES=$(exec ./describe_instances.sh regions.txt | grep "INSTANCE")

if [ -e cloud_ips.txt ]; then
  rm cloud_ips.txt
fi

running=()
instance_ips=()
IFS=$'\n'
for instance in $INSTANCES
do
#  echo "Instance: "$instance 
  IFS=$OLDIFS
  set $instance
  instance_id=$2
  instance_state=$4
  if [ "$4" != "stopped" ]; then
    echo "Instance: "$instance 
    echo "Instance is running: $4"
    running[${#running[@]}]=true
    instance_ips[${#instance_ips[@]}]=${14}
  else
    echo "Instance is not running: $4"
    running[${#running[@]}]=false
    instance_ips[${#instance_ips[@]}]="-"
  fi
  IFS=$'\n'
done

# iterate over region ids and instances simultaneously
echo "Region count: ${#region_ids[@]}"
for ((i=0; i < ${#region_ids[@]}; i++))
do
  is_running=${running[$i]}
  if $is_running ; then
    echo "Running:  ${region_ids[$i]} IP: ${instance_ips[$i]} "
    echo "${region_ids[$i]} ${instance_ips[$i]}" >> cloud_ips.txt
  else
    echo "Not running"
  fi
done


