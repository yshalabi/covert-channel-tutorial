add_executable (pp-llc-send sender.c)
target_link_libraries (fr-send LINK_PUBLIC util)
add_executable (pp-llc-recv receiver.c)
target_link_libraries (fr-recv LINK_PUBLIC util)
