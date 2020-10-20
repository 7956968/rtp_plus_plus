#!/bin/bash

regions=$(cat $1)
OLDIFS=$IFS

IFS=$'\n'

for region in $regions
do
  IFS=$OLDIFS
  set $region
  echo "URL: $3 Instance: $4"
  URL=$3
  export EC2_URL=https://$URL
  ec2-describe-instances
done

