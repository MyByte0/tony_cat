cmake_minimum_required(VERSION 3.7)

project(tony_cat CXX)


ADD_DEFINITIONS("-DASIO_STANDALONE")
ADD_DEFINITIONS("-DGLOG_NO_ABBREVIATED_SEVERITIES")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FIND_PACKAGE(Threads REQUIRED)

SET(CMAKE_CXX_STANDARD 20)

ADD_DEFINITIONS("-DPROJ_CXX_STANDARD=${CMAKE_CXX_STANDARD}")


# compile argments setup
if(CMAKE_COMPILER_IS_GNUCXX)
	SET(CMAKE_CXX_FLAGS "-fcoroutines ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g")
	SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os")
elseif(MSVC)
    # example: CMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake)
	INCLUDE(${CMAKE_TOOLCHAIN_FILE})
	ADD_DEFINITIONS("-D_WIN32_WINNT=0x0601")
	ADD_DEFINITIONS("-DWIN32_LEAN_AND_MEAN")
	add_compile_options(/wd4251)
	add_compile_options(/wd4996)
endif()

# config database
option(USE_ROCKSDB "option for rocksdb" OFF)
if (USE_ROCKSDB) 
    add_definitions(-DUSE_ROCKSDB)
endif(USE_ROCKSDB)

option(USE_MYSQL "option for mysql" OFF)
if (USE_MYSQL) 
    add_definitions(-DUSE_MYSQL)
endif(USE_MYSQL)

if((NOT USE_ROCKSDB) AND (NOT USE_MYSQL))
	# default use mysql
	MESSAGE(STATUS "default use mysql")
	set(USE_MYSQL ON)
	ADD_DEFINITIONS(-DUSE_MYSQL)
endif()

if(USE_ROCKSDB)
	MESSAGE(STATUS "use database rocksdb")

	if(CMAKE_COMPILER_IS_GNUCXX)
		SET(LIB_DEPEND
		${LIB_DEPEND}
		"-lrocksdb"
		)
	elseif(MSVC)
		FIND_PACKAGE(RocksDB REQUIRED)
		SET(LIB_DEPEND
		${LIB_DEPEND}
		RocksDB::rocksdb
		)
	endif()
endif()

if(USE_MYSQL)
	MESSAGE(STATUS "use database mysql")
	if(CMAKE_COMPILER_IS_GNUCXX)
		SET(LIB_DEPEND
		${LIB_DEPEND}
		"-lmysqlclient"
		)
	elseif(MSVC)
		FIND_PACKAGE(libmysql REQUIRED)
		SET(LIB_DEPEND
		${LIB_DEPEND}
		${MYSQL_LIBRARIES}
		)
	endif()
endif()

# common lib
FIND_PACKAGE(glog REQUIRED)
FIND_PACKAGE(tinyxml2 REQUIRED)
FIND_PACKAGE(fmt)

SET(LIB_DEPEND
	${LIB_DEPEND}
	glog::glog
	tinyxml2::tinyxml2
	fmt::fmt
)


# load lib protobuf
if(CMAKE_COMPILER_IS_GNUCXX)
	FIND_PACKAGE(Protobuf REQUIRED)
	SET(LIB_DEPEND
	${LIB_DEPEND}
	${Protobuf_LIBRARIES}
	)
elseif(MSVC)
    FIND_PACKAGE(Protobuf CONFIG REQUIRED)
	SET(LIB_DEPEND
		${LIB_DEPEND}
		protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
	)
endif()


# handle script cmd
if(CMAKE_COMPILER_IS_GNUCXX)
	EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protocol
					COMMAND sh pb_gen_code.sh
					)
elseif(MSVC)
	EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protocol
					COMMAND ${Protobuf_PROTOC_EXECUTABLE} *.proto --cpp_out=.
					)
endif()


# build output
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)


# add logic pb files
FILE(GLOB_RECURSE COMMON_FILES ./common/*.h ./common/*.cc ./common/*.hpp ./common/*.cpp)
FILE(GLOB_RECURSE PROTOBUF_FILES ./protocol/*.h ./protocol/*.cc )

# library define
ADD_LIBRARY(TONY_CAT
	${COMMON_FILES}
)

ADD_LIBRARY(PROTO
	${PROTOBUF_FILES}
)


# add sub directory
ADD_SUBDIRECTORY(
	example
)
