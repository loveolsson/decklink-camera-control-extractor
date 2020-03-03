cmake_minimum_required(VERSION 3.2.2)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	include_directories(
		3rdParty/macos/
	)

	file(GLOB OS_SOURCES
		3rdParty/macos/include/DeckLinkAPIDispatch.cpp
	)

	add_compile_definitions(MACOS)
	find_library(CORE_FOUNDATION CoreFoundation)
	if (NOT CORE_FOUNDATION)
		message(FATAL_ERROR "CoreFoundation not found")
	endif()
else()
	include_directories(
		3rdParty/linux/
	)

	file(GLOB OS_SOURCES
		3rdParty/linux/include/DeckLinkAPIDispatch.cpp
	)

	add_compile_definitions(LINUX)
endif()


file(GLOB SOURCES
	src/serialoutput.h
	src/serialoutput.cpp
	src/printpacket.h
	src/printpacket.cpp
	src/mutexfifo.h
	src/main.cpp
	src/defines.h
	src/decklink.h
	src/decklink.cpp
	src/commands.h
	src/commands.cpp
	src/dlwrapper.h
)

# add the executable
add_executable(DeckLinkCameraControl ${SOURCES} ${OS_SOURCES})
target_link_libraries(DeckLinkCameraControl pthread ${CMAKE_DL_LIBS} ${CORE_FOUNDATION})
