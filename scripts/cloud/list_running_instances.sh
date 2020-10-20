#!/bin/bash

regions=$(cat regions.txt)
OLDIFS=$IFS

IFS=$'\n'

INSTANCES=$(exec ./describe_instances.sh regions.txt | grep "INSTANCE")

for instance in $INSTANCES
do
#  echo "Instance: "$instance 
  IFS=$OLDIFS
  set $instance
  instance_id=$2
  instance_state=$4
  if [ $4 != "stopped" ]; then
    echo "Instance is running: $4"
  fi
  IFS=$'\n'
done

