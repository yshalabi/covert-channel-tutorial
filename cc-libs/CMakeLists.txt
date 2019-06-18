add_library (l1d l1.c)
target_include_directories (l1d PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library (util util.c)
target_include_directories (util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
