#!/bin/bash

echo "./create_deployment.sh"

if [ -d rtp++ ]; then
  rm -fr rtp++
fi
mkdir rtp++

if $binaries ; then
  echo "copying binaries"
  cp ../bin/Rtp* rtp++
fi
if $dependencies ; then
  echo "copying dependencies"
  cp ../deploy/dependencies/* rtp++
fi

echo "copying scripts"
# cp copy_logs.sh rtp++

echo "Copying config"
mkdir rtp++/etc
cp ../bin/*.sdp rtp++/etc
cp ../bin/*.cfg rtp++/etc
cp ../bin/ec2_tests/* rtp++/etc

