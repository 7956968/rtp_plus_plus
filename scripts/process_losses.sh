#!/bin/bash
files=$(ls *.log* | grep 'RtpReceiver')
echo $files

for i in $files
do
  ./analyseLosses.sh $i 
done
