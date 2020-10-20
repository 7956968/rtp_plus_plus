#!/bin/bash

count=$(cat regions.txt | wc -l)
./start_instances.sh regions.txt
echo "Starting $count instances"

# wait for machines to boot
sleep 30

started=$(./list_running_instances.sh | wc -l)

if [ $count -ne $started ]; then
  echo "$started/$count Could not start all instances, check manually before continuing"
  exit 0
else
  echo "$started/$count instances started successfully"  
fi

./get_instance_ips.sh
echo "IPs are now in cloud_ips.txt"

# generate configuration files

IPS=$(cat cloud_ips.txt)

OLDIFS=$IFS

IFS=$'\n'

cp mst.template mst.txt
for line in $IPS
do
  IFS=$OLDIFS
  set $line
  sed -i "s/$1/$2/g" mst.txt
  IFS=$'\n'
done

echo "MST generated"

