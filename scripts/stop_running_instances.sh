#!/bin/bash

regions=$(cat regions.txt)
OLDIFS=$IFS

IFS=$'\n'

INSTANCES=$(exec ./describe_instances.sh regions.txt | grep "INSTANCE")
#INSTANCES=$(exec ./describe_instances.sh regions.txt)
#echo "Instances: "$INSTANCES

for instance in $INSTANCES
do
#  echo "Instance: "$instance 
  IFS=$OLDIFS
  set $instance
  instance_id=$2
  instance_state=$4
  echo "Instance id: $2 State: $4"
  if [ $4 != "stopped" ]; then
    echo "Instance is running: $4 - stopping"
    region="${11:0:${#11}-1}"
    echo "Region: $region" 
    ec2stop $2 --region $region
  fi
  IFS=$'\n'
done

