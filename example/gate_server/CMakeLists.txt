cmake_minimum_required(VERSION 3.7)

SET(GATE_SERVER_FILES
	client_pb_module.h
	client_pb_module.cpp
	main.cpp
)

ADD_EXECUTABLE(GateServer 
	${GATE_SERVER_FILES}
)

target_link_libraries (GateServer TONY_CAT ${LIB_DEPEND})
