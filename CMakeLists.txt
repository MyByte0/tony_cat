cmake_minimum_required(VERSION 3.7)

project(tony_cat CXX)


ADD_DEFINITIONS("-DASIO_STANDALONE")
ADD_DEFINITIONS("-DGLOG_NO_ABBREVIATED_SEVERITIES")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FIND_PACKAGE(Threads REQUIRED)

SET(CMAKE_CXX_STANDARD 20)

ADD_DEFINITIONS("-DPROJ_CXX_STANDARD=${CMAKE_CXX_STANDARD}")

SET(COMPILER_IS_GNU_LIKELY ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang")))

# compile argments setup
if(COMPILER_IS_GNU_LIKELY)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines ")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 ")
    endif()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter ")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -fsanitize=address ")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Os")
elseif(MSVC)
    # example: CMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake)
    INCLUDE(${CMAKE_TOOLCHAIN_FILE})
    INCLUDE_DIRECTORIES(${VCPKG_INSTALLED_DIR}/x64-windows/include)
    ADD_DEFINITIONS("-D_WIN32_WINNT=0x0601")
    ADD_DEFINITIONS("-DWIN32_LEAN_AND_MEAN")
    ADD_DEFINITIONS("-DNOMINMAX")
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
    MESSAGE(STATUS "default use rocksdb")
    set(USE_ROCKSDB ON)
    ADD_DEFINITIONS(-DUSE_ROCKSDB)
endif()

if(USE_ROCKSDB)
    MESSAGE(STATUS "use database rocksdb")

    if(COMPILER_IS_GNU_LIKELY)
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
    if(COMPILER_IS_GNU_LIKELY)
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
if(COMPILER_IS_GNU_LIKELY)
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
if(CMAKE_HOST_WIN32)
    EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protocol
        COMMAND ${Protobuf_PROTOC_EXECUTABLE} *.proto --cpp_out=.
    )
    EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools
        COMMAND .\\db_gen.bat
        COMMAND .\\cfg_gen.bat
        OUTPUT_VARIABLE none
    )
else()
    EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protocol
        COMMAND sh pb_gen_code.sh
    )
    EXECUTE_PROCESS(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools
        COMMAND sh db_gen.sh
        COMMAND sh cfg_gen.sh
        OUTPUT_VARIABLE none
    )
endif()


# build output
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)


# add logic pb files
FILE(GLOB_RECURSE COMMON_FILES ./common/*.h ./common/*.cc ./common/*.hpp ./common/*.cpp)
FILE(GLOB_RECURSE BASE_PROTOBUF_FILES ./protocol/*.h ./protocol/*.cc )

# library define
ADD_LIBRARY(TONY_CAT
    ${COMMON_FILES}
)

ADD_LIBRARY(BASE_PROTO
    ${BASE_PROTOBUF_FILES}
)


# add sub directory
ADD_SUBDIRECTORY(
    app
)
