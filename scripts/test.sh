#!/bin/bash

cmd_start=false
cmd_stop=true

if $cmd_start; then
  echo `eval date +%Y%m%d"_"%H_%M_%S` > t_start.txt
elif $cmd_stop; then
  echo `eval date +%Y%m%d"_"%H_%M_%S` > t_stop.txt
fi

