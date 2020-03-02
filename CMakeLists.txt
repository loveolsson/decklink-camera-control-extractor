cmake_minimum_required(VERSION 3.2.2)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES
	src/main.cpp
	src/decklink.h
	src/decklink.cpp
	3rdPary/include/DeckLinkAPI.h
	3rdPary/include/DeckLinkAPIDispatch.cpp
)

include_directories(
	3rdParty/
)

# add the executable
add_executable(DeckLinkCameraControl ${SOURCES})
