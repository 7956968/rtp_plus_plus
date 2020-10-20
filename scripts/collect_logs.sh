#!/bin/bash

IPS=()
SESSIONS=()

# read config file line by line
while read line
do
#  echo $line
#split each line according to spaces
  SESSIONS[${#SESSIONS[@]}]=$line
  set $line
# get src dst and ports
#  echo "ID: "$1" SRC: "$2":"$3" DST: "$4":"$5" BW: "$6"kbps"

  IPS[${#IPS[@]}]=$2
  IPS[${#IPS[@]}]=$4
done

# removes duplicate entries from IP list
UNIQUE_IPS=( $( printf "%s\n" "${IPS[@]}" | awk 'x[$0]++ == 0' ) )

# print IP count
echo "Printing IPs: "${#IPS[@]}
echo "Unique IPs: "${#UNIQUE_IPS[@]}


# generate deployment archive for each host
# which includes binaries, dependencies,
# scripts and configuration files
# one deployment archive should contain 
# config files related to both hosts
# create archive

mkdir -p logs
mkdir -p logs/archive
# archive old logs
mv logs/log_*.tar.gz logs/archive

for uip in "${UNIQUE_IPS[@]}"
do  
# change IFS for cat
  OLD_IFS=$IFS
  IFS=$'\n'

  user_names=$(cat config/host_user.txt)

  user_name=""
  for host_user in $user_names 
  do
# reset IFS for rest of program
    IFS=$OLD_IFS
    set $host_user
#    echo "Checking $uip-$hostuser: $1-$2"
    if [ $1 = $uip ]; then
      user_name=$2
    fi
    IFS=$'\n'
  done

# reset IFS for rest of program
  IFS=$OLD_IFS


  echo "Copying logs from destination: $user_name@$uip"
  if [ -n "$user_name" ]; then
    scp $user_name@$uip:~/rtp++/log.tar.gz logs/log_$uip.tar.gz
  else
    scp -i ~/.ssh/cloud.pem ubuntu@$uip:~/rtp++/log.tar.gz logs/log_$uip.tar.gz
  fi
done

