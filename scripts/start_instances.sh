#!/bin/bash

regions=$(cat $1)
OLDIFS=$IFS

IFS=$'\n'

if [ -e temp_ips.txt ]; then
  rm temp_ips.txt
fi

for region in $regions
do
  IFS=$OLDIFS
  set $region
  echo "URL: $3 Instance: $4"
  URL=$3
  export EC2_URL=https://$URL
  ec2-start-instances $4 | grep "INSTANCE" >> temp_ips.txt
done

cloud_ips=$(cat temp_ips.txt)


IFS=$'\n'
for line in $cloud_ips
do
  IFS=$OLDIFS
  set $line
  ext_ip=${13}
  echo $ext_ip >> cloud_ips.txt 
done
