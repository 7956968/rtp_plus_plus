#!/bin/bash

ip=$(ifconfig eth0 | grep inet | awk '{print $2}' | sed 's/addr://')
echo "IP "$ip 
hosts=$(cat hosts.txt)

for host in $hosts
do
  if [ -n "$host" ]; then
    if [ $ip != $host ]; then
      pf=$ip"_"$host"_p.txt"
      trf=$ip"_"$host"_tr.txt"
      ping -c 15 $host > $pf
      traceroute $host > $trf
#      scp $pf rglobisch@130.149.228.93:~/data/import/$pf
#      scp $trf rglobisch@130.149.228.93:~/data/import/$trf
    fi
  fi
done

