#!/bin/bash
cd protocol

protoc --cpp_out=./ `find ./ -name "*proto"`