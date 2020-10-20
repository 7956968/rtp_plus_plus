#!/bin/bash
files=$(ls *.log* | grep 'RtpSender')
echo $files

for i in $files
do
  ./generate_plot.sh $i 
done
