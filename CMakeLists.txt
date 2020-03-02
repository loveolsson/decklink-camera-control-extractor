cmake_minimum_required(VERSION 3.2.2)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES
	src/main.cpp
	3rdPary/include/DeckLinkAPI.h
	3rdPary/include/DeckLinkAPIDispatch.cpp
)

# add the executable
add_executable(DeckLinkCameraControl ${SOURCES})


