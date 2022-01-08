cmake_minimum_required(VERSION 3.7)

project(tony_cat CXX)


add_definitions("-DASIO_STANDALONE")
add_definitions("-DGLOG_NO_ABBREVIATED_SEVERITIES")
include_directories(thirdparty/asio/asio/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

FIND_PACKAGE(Threads REQUIRED)

SET(CMAKE_CXX_STANDARD 20)

add_definitions("-DPROJ_CXX_STANDARD=${CMAKE_CXX_STANDARD}")

if(MSVC)
       # example: CMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake)
	   include(${CMAKE_TOOLCHAIN_FILE})
	   #SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} /MTd")
	   #SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} /MTd")
	   add_definitions("-D_WIN32_WINNT=0x0601")
	   add_compile_options(/wd4251)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-fcoroutines ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g")
	SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	find_package(fmt)
	set(LIB_DEPEND
		${LIB_DEPEND}
		fmt::fmt
	   )
endif()

FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(glog REQUIRED)
FIND_PACKAGE(tinyxml2 REQUIRED)

set(LIB_DEPEND
	${LIB_DEPEND}
	glog::glog
	tinyxml2::tinyxml2
)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(LIB_DEPEND
	${LIB_DEPEND}
	${Protobuf_LIBRARIES}
	)
endif()

if(MSVC)
	set(LIB_DEPEND
		${LIB_DEPEND}
		protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
	)
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

file(GLOB_RECURSE COMMON_FILES ./common/*.h ./common/*.cc ./common/*.hpp ./common/*.cpp)
file(GLOB_RECURSE LOG_FILES ./log/*.h ./log/*.cc ./log/*.hpp ./log/*.cpp)
file(GLOB_RECURSE NET_FILES ./net/*.h ./net/*.cc ./net/*.hpp ./net/*.cpp)
file(GLOB_RECURSE PROTOBUF_FILES ./protocol/*.h ./protocol/*.cc )

ADD_LIBRARY(TONY_CAT
	${NET_FILES}
	${LOG_FILES}
	${COMMON_FILES}
)

ADD_LIBRARY(NET_PB
	${PROTOBUF_FILES}
)

ADD_SUBDIRECTORY(
	example
)
