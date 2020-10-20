#!/bin/bash

regions=$(cat regions.txt)
OLDIFS=$IFS

IFS=$'\n'

INSTANCES=$(./describe_instances.sh regions.txt | grep "INSTANCE")

not_running=true
for instance in $INSTANCES
do
#  echo "Instance: "$instance 
  IFS=$OLDIFS
  set $instance
  instance_id=$2
  instance_state=$4
  if [ $4 != "stopped" ]; then
    echo "Instance is running: $4"
    not_running=false
  fi
  IFS=$'\n'
done

# Don't print if no running instances: we pipe the output into wc to count them
#if $not_running ; then
#  echo "No running instances"
#  exit 0
#fi
