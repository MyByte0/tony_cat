cmake_minimum_required(VERSION 3.7)

FILE(GLOB_RECURSE LOGIC_SERVER_FILES ./*.h ./*.cc ./*.hpp ./*.cpp)

ADD_EXECUTABLE(LogicServer 
	${LOGIC_SERVER_FILES}
)

TARGET_LINK_LIBRARIES(LogicServer APP_GAME_PROTO TONY_CAT BASE_PROTO ${LIB_DEPEND})
