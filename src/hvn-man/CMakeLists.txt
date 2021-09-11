###########
# hvn-man #
###########

add_executable(hvn-man main.c)
target_include_directories(hvn-man PRIVATE ${CMARK_INCLUDE_DIRS})
target_link_directories(hvn-man PRIVATE ${CMARK_LIBRARY_DIRS})
target_link_libraries(hvn-man PRIVATE ${CMARK_LIBRARIES})
