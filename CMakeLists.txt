cmake_minimum_required(VERSION 3.9)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 17)
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
	SET(CPACK_GENERATOR "productbuild")
	SET(CPACK_PRODUCTBUILD_BACKGROUND "")
else()
	include_directories(
		3rdParty/linux/
	)

	file(GLOB OS_SOURCES
		3rdParty/linux/include/DeckLinkAPIDispatch.cpp
	)

	add_compile_definitions(LINUX)
	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
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
set_property(TARGET DeckLinkCameraControl PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)

install(TARGETS DeckLinkCameraControl DESTINATION bin)

INCLUDE(CPack)
