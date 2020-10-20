#!/bin/bash

numargs=$#
binaries=false
dependencies=false

# check if we should bundle dependencies and binaries
for ((i=1 ; i <= $numargs ; i++))
do
#  echo "Arg $i: $1"
  if [ $1 = "-h" ]; then
    echo "Usage: cat mst.txt | generate.sh"
    echo "Optional parameters:"
    echo "-d deploy dependencies"
    echo "-b deploy binaries"
    exit 0
  fi
  if [ $1 = "-d" ]; then
    dependencies=true
  fi
  if [ $1 = "-b" ]; then
    binaries=true
  fi
  shift
done

echo "Dependencies: $dependencies"
echo "Binaries: $binaries"

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
#  echo "ID: "$1" SRC: "$2":"$3" DST: "$4":"$5" BW: "$6"kbps"

    IPS[${#IPS[@]}]=$2
    IPS[${#IPS[@]}]=$4

# create source sdp

    sed "s/<<src>>/$2/g" templates/template.sdp > $1".sdp"
    sed -i "s/<<port>>/$3/g" $1.sdp
    sed -i "s/<<bw>>/$6/g" $1.sdp

# create local ports for receiver
    sed "s/<<dst>>/$4/g" templates/template.cfg > $1".cfg"
    sed -i "s/<<rtp-port>>/$5/g" $1.cfg
    let rtcp_port=$5+1
#  echo "RTCP: "$rtcp_port
    sed -i "s/<<rtcp-port>>/$rtcp_port/g" $1.cfg
  fi
done

# create deployment buckets based on IP addresses
# echo "Printing IPs: "${#IPS[@]}

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

# remove files if exist
  for i in "${UNIQUE_HOSTS[@]}"
  do
    filename=$uip"_hosts.txt"
    if [ -e $filename ]; then
      echo "$filename exists- deleting"
      rm $filename
    fi
  done

# append unique hosts to newly created file
  for i in "${UNIQUE_HOSTS[@]}"
  do
    filename=$uip"_hosts.txt"
    echo $i >> $filename
  done

done

mkdir -p archives
mkdir -p connect
# generate deployment archive for each host
# which includes binaries, dependencies,
# scripts and configuration files
# one deployment archive should contain 
# config files related to both hosts
# create archive
count=0
for uip in "${UNIQUE_IPS[@]}"
do
  if [ -d rtp++ ]; then
#    echo "Removing old gen"
    rm -fr rtp++
  fi
  mkdir rtp++
  
  echo "Generating deployment archive for $uip"
  if $binaries ; then
    echo "copying binaries"
    cp ../bin/Rtp* rtp++
  fi
  if $dependencies ; then
    echo "copying dependencies"
    cp ../deploy/dependencies/* rtp++
  fi
#  echo "copying scripts"
  cp copy_logs.sh rtp++
#  echo "Copying config"

  echo '#!/bin/bash' >> run.sh
  echo 'killall RtpSender' >> run.sh
  echo 'killall RtpReceiver' >> run.sh
  echo 'if [ -d log ]; then rm -fr log ; fi' >> run.sh

  echo '#!/bin/bash' >> stop.sh
  echo 'killall RtpSender' >> stop.sh
  echo 'killall RtpReceiver' >> stop.sh
  echo 'tar -czf log.tar.gz log' >> stop.sh

  for session in "${SESSIONS[@]}"
  do
    cp $uip"_hosts.txt" rtp++
    found=$(echo $session | grep $uip)
    if [ -n "$found" ]; then
      set $found
      cp $1".sdp" rtp++
      cp $1".cfg" rtp++

# create master script that starts senders and receivers
# if we are the sender, add sender script
# else add receiver
      if [ $uip = $4 ]; then
# we are the receiver
        sed "s/<<session>>/$1/g" templates/receiver.template > $1"_receiver.sh"
        chmod 755 $1"_receiver.sh"
        cp $1"_receiver.sh" rtp++
        echo './'$1'_receiver.sh &' >> run.sh
      else
# we are the sender
        sed "s/<<session>>/$1/g" templates/sender.template > $1"_sender.sh"
        chmod 755 $1"_sender.sh"
        cp $1"_sender.sh" rtp++
        echo './'$1'_sender.sh &' >> run.sh
      fi

# traces
      
      sed "s/<<IP>>/$uip/g" templates/run_traces.template > "run_traces.sh"
      cp run_traces.sh rtp++
      cp tr.sh rtp++
    fi
  done
  
  chmod 755 run.sh
  mv run.sh rtp++

  chmod 755 stop.sh
  mv stop.sh rtp++

#  echo "creating archive"
  tar -czf archives/rtp++_$uip.tar.gz rtp++

# copy archive to target machine: known users in host_user.txt else default user ubuntu is assumed with default key cloud.pem
  
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

  echo "tar -xvzf rtp++_$uip.tar.gz" > x.sh
  chmod 755 x.sh
  echo "Copying archive to destination: $user_name@$uip"
  if [ -n "$user_name" ]; then
    scp archives/rtp++_$uip.tar.gz $user_name@$uip:~/data/rtp++_$uip.tar.gz
#    scp x.sh $user_name@$uip:~/
    echo "ssh $user_name@$uip" > connect/c$count"_"$uip.sh
    sed "s/<<user>>/$user_name/g" templates/update.template > "update.sh"
    chmod 755 update.sh
    scp update.sh $user_name@$uip:~/scripts/update.sh
  else
    scp -i ~/.ssh/cloud.pem archives/rtp++_$uip.tar.gz ubuntu@$uip:~/data/rtp++_$uip.tar.gz
#    scp -i ~/.ssh/cloud.pem x.sh ubuntu@$uip:~/
    echo "ssh -i ~/.ssh/cloud.pem ubuntu@$uip" > connect/c$count"_"$uip.sh
    sed "s/<<user>>/ubuntu/g" templates/update.template > "update.sh"
    chmod 755 update.sh
    scp -i ~/.ssh/cloud.pem update.sh ubuntu@$uip:~/scripts/update.sh
  fi
  chmod 755 connect/c$count"_"$uip.sh
  rm x.sh
  rm update.sh
  let count=$count+1
done

####################### clean up #######################
if [ -d rtp++ ]; then
  echo "Removing rtp++"
  rm -fr rtp++
fi

echo "Removing host files"
for uip in "${UNIQUE_IPS[@]}"
do
  rm $uip"_hosts.txt"
done

echo "Removing SDP files"
for session in "${SESSIONS[@]}"
do
  set $session
  echo "Removing "$1".sdp cfg _sender_sh _receiver.sh"
  rm $1".sdp"
  rm $1".cfg"
  rm $1"_sender.sh"
  rm $1"_receiver.sh"
done


