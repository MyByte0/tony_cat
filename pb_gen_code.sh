#!/bin/bash
cd protocol

protoc --cpp_out=./ ./db_data.proto

protoc --cpp_out=./ ./server_base.proto
protoc --cpp_out=./ ./server_common.proto

protoc --cpp_out=./ ./server_db.proto

protoc --cpp_out=./ ./client_base.proto
protoc --cpp_out=./ ./client_common.proto