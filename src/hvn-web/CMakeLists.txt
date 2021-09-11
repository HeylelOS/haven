###########
# hvn-web #
###########

add_executable(hvn-web main.c)
target_include_directories(hvn-web PRIVATE ${CMARK_INCLUDE_DIRS})
target_link_directories(hvn-web PRIVATE ${CMARK_LIBRARY_DIRS})
target_link_libraries(hvn-web PRIVATE ${CMARK_LIBRARIES})
