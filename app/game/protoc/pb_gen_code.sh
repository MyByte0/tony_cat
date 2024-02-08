#!/bin/bash
protoc --cpp_out=./ `find ./ -name "*.proto"`