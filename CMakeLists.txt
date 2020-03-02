cmake_minimum_required(VERSION 3.2.2)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


file(GLOB SOURCES
	3rdParty/include/DeckLinkAPI.h
	3rdParty/include/DeckLinkAPIDispatch.cpp
	src/main.cpp
	src/decklink.h
	src/decklink.cpp
)

include_directories(
	3rdParty/
)

# add the executable
add_executable(DeckLinkCameraControl ${SOURCES})
target_link_libraries(DeckLinkCameraControl ${CMAKE_DL_LIBS})
