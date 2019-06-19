add_executable (pp-llc-send sender.c)
target_link_libraries (pp-llc-send LINK_PUBLIC util)
add_executable (pp-llc-recv receiver.c)
target_link_libraries (pp-llc-recv LINK_PUBLIC util)
