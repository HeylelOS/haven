cmake_minimum_required(VERSION 3.14)

project(Haven LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CMARK REQUIRED libcmark)

include_directories(${CMARK_INCLUDE_DIRS})
link_directories(${CMARK_LIBRARY_DIRS})

file(GLOB_RECURSE HVN_MAN_SOURCES CONFIGURE_DEPENDS src/hvn-man/*.c)
add_executable(hvn-man ${HVN_MAN_SOURCES})

file(GLOB_RECURSE HVN_WEB_SOURCES CONFIGURE_DEPENDS src/hvn-web/*.c)
add_executable(hvn-web ${HVN_WEB_SOURCES})

target_link_libraries(hvn-man PUBLIC ${CMARK_LIBRARIES})
target_link_libraries(hvn-web PUBLIC ${CMARK_LIBRARIES})

