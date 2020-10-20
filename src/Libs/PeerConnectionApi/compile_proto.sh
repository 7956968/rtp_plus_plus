#!/bin/bash

protoc -I . --cpp_out=. peer_connection_api.proto
protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` peer_connection_api.proto

