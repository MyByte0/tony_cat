cmake_minimum_required(VERSION 3.7)

SET(LOGIC_SERVER_FILES
	main.cpp
)

ADD_EXECUTABLE(LogicServer 
	${LOGIC_SERVER_FILES}
)

target_link_libraries (LogicServer TONY_CAT ${LIB_DEPEND})
