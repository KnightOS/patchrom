cmake_minimum_required(VERSION 2.8.5)
project(patchrom C)
set(CMAKE_C_FLAGS "-g -Wall")

add_executable(patchrom main.c)

install(PROGRAMS ${CMAKE_BINARY_DIR}/patchrom DESTINATION bin)

if (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

ADD_CUSTOM_COMMAND(
  TARGET patchrom
  DEPENDS ${CMAKE_SOURCE_DIR}/patchrom.1.txt
  COMMAND a2x --no-xmllint --doctype manpage --format manpage ${CMAKE_SOURCE_DIR}/patchrom.1.txt -D ${CMAKE_BINARY_DIR} -v
  OUTPUTS ${CMAKE_BINARY_DIR}/patchrom.1
)

INSTALL(FILES ${CMAKE_BINARY_DIR}/patchrom.1 DESTINATION man/man1)
