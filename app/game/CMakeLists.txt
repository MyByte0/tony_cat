cmake_minimum_required(VERSION 3.7)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# handle protoc
if(CMAKE_COMPILER_IS_GNUCXX)
	EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protoc
					COMMAND sh pb_gen_code.sh
					)
elseif(MSVC)
	EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protoc
					COMMAND ${Protobuf_PROTOC_EXECUTABLE} *.proto --cpp_out=.
					)
endif()
FILE(GLOB_RECURSE APP_GAME_PROTOBUF_FILES ./protoc/*.h ./protoc/*.cc )

ADD_LIBRARY(APP_GAME_PROTO
	${APP_GAME_PROTOBUF_FILES}
)

ADD_SUBDIRECTORY(
  client
)

ADD_SUBDIRECTORY(
  gate_server
)

ADD_SUBDIRECTORY(
  logic_server
)

ADD_SUBDIRECTORY(
  db_server
)