cmake_minimum_required(VERSION 3.14)

project(Haven LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CMARK REQUIRED libcmark)

add_subdirectory(src/hvn-man)
add_subdirectory(src/hvn-web)

###########
# Install #
###########

install(TARGETS hvn-man hvn-web RUNTIME COMPONENT Runtime)
install(TARGETS hvn-man hvn-web RUNTIME COMPONENT CommandLineInterface)

add_custom_target(man1 ALL DEPENDS man1/hvn-man.1 man1/hvn-web.1)
add_custom_command(OUTPUT man1/hvn-man.1 COMMAND mkdir -p man1 COMMAND hvn-man -o man1/hvn-man.1 -i ${PROJECT_SOURCE_DIR}/docs/hvn-man.1.md)
add_custom_command(OUTPUT man1/hvn-web.1 COMMAND mkdir -p man1 COMMAND hvn-man -o man1/hvn-web.1 -i ${PROJECT_SOURCE_DIR}/docs/hvn-web.1.md)

install(DIRECTORY ${PROJECT_BINARY_DIR}/man1 TYPE MAN COMPONENT CommandLineInterface)
