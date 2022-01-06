cmake_minimum_required(VERSION 3.7)

SET(EXAMPLE_FILES
	main.cpp
)

ADD_EXECUTABLE(demo1 
	${EXAMPLE_FILES}
)

target_link_libraries (demo1 NET_PB TONY_CAT ${LIB_DEPEND})
