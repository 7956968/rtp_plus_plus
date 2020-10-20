#!/bin/bash

protoc -I . --cpp_out=. remote_cloud_control.proto
protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` remote_cloud_control.proto

