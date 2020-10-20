#!/bin/bash

# script archives previously created rtp++ folder
# a parameter can be passed in for naming of the archive
echo "./create_deployment_archive.sh archives/rtp++_$1.tar.gz"
tar -czf archives/rtp++_$1.tar.gz rtp++

