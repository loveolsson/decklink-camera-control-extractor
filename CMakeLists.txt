cmake_minimum_required(VERSION 3.9)

# set the project name
project(DeckLinkCameraControl)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	include_directories(
		3rdParty/macos/
		include/
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
		include/
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
	src/serialoutput.cpp
	src/printpacket.cpp
	src/main.cpp
	src/decklink.cpp
	src/commands.cpp
)

# add the executable
add_executable(DeckLinkCameraControl ${SOURCES} ${OS_SOURCES})
target_link_libraries(DeckLinkCameraControl pthread ${CMAKE_DL_LIBS} ${CORE_FOUNDATION})
set_property(TARGET DeckLinkCameraControl PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)

SET(CPACK_PACKAGE_CONTACT "Love Olsson (love@loveolsson.se")
install(TARGETS DeckLinkCameraControl DESTINATION bin)
INCLUDE(CPack)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(DLWRAPPER_LOGGING)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

