add_executable (fr-send sender.c)
target_link_libraries (fr-send LINK_PUBLIC fr_util)
add_executable (fr-recv receiver.c)
target_link_libraries (fr-recv LINK_PUBLIC fr_util)
