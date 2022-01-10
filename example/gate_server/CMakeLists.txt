cmake_minimum_required(VERSION 3.7)

SET(GATE_SERVER_FILES
	main.cpp
)

ADD_EXECUTABLE(GateServer 
	${GATE_SERVER_FILES}
)

target_link_libraries (GateServer TONY_CAT ${LIB_DEPEND})
