cmake_minimum_required(VERSION 3.7)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

ADD_SUBDIRECTORY(
  gate_server
)

ADD_SUBDIRECTORY(
  logic_server
)
