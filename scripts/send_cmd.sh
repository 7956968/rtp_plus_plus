#!/bin/bash

numargs=$#
cmd_start=false
cmd_stop=false
cmd_update=false

# check if we should bundle dependencies and binaries
for ((i=1 ; i <= $numargs ; i++))
do
#  echo "Arg $i: $1"
  if [ $1 = "-h" ]; then
    echo "Usage: cat mst.txt | start_rtp.sh"
    echo "Optional parameters:"
    echo "-start"
    echo "-stop"
    echo "-update"
    exit 0
  fi
  if [ $1 = "-start" ]; then
    cmd_start=true
  fi
  if [ $1 = "-stop" ]; then
    cmd_stop=true
  fi
  if [ $1 = "-update" ]; then
    cmd_update=true
  fi
  shift
done

file=""

if $cmd_update ; then
  file="UPDATE.cmd"
fi
if $cmd_stop; then
  file="STOP.cmd"
fi
if $cmd_start ; then
  file="START.cmd"
fi

IPS=()
SESSIONS=()


# read config file line by line
while read line
do
#  echo "Read "$line

  if [ -n "$line" ]; then
#split each line according to spaces
    SESSIONS[${#SESSIONS[@]}]=$line
    set $line
# get src dst and ports

    IPS[${#IPS[@]}]=$2
    IPS[${#IPS[@]}]=$4
  fi
done

# removes duplicate entries from IP list
UNIQUE_IPS=( $( printf "%s\n" "${IPS[@]}" | awk 'x[$0]++ == 0' ) )

# print IP count
echo "Printing IPs: "${#IPS[@]}
echo "Unique IPs: "${#UNIQUE_IPS[@]}

# go through each session and store IP
for uip in "${UNIQUE_IPS[@]}"
do
#  echo "Searching for hosts connected to $uip"
  HOSTS=()
  for session in "${SESSIONS[@]}"
  do
#    echo $session 
    found=$(echo $session | grep $uip)
    if [ -n "$found" ]; then
#      echo "Found match: $found"
      set $found
      if [ $uip = $4 ]; then
        host=$2
        HOSTS[${#HOSTS[@]}]=$host
      else
        host=$4
        HOSTS[${#HOSTS[@]}]=$host
      fi
    fi
  done
  UNIQUE_HOSTS=( $( printf "%s\n" "${HOSTS[@]}" | awk 'x[$0]++ == 0' ) )

done

count=0
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

  touch $file

  echo "Copying command to destination: $user_name@$uip"
  if [ -n "$user_name" ]; then
    scp $file $user_name@$uip:~/data/$file
  else
    scp -i ~/.ssh/cloud.pem $file ubuntu@$uip:~/data/$file
  fi
  let count=$count+1
done

if $cmd_start; then
  echo `eval date +%Y%m%d"_"%H_%M_%S` > t_start.txt
elif $cmd_stop; then
  echo `eval date +%Y%m%d"_"%H_%M_%S` > t_stop.txt
fi

