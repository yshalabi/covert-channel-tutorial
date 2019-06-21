add_library (util util.c)
add_library (fr_util fr_util.c)
target_include_directories (util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (fr_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
