cmake_minimum_required(VERSION 3.12)
include(ExternalProject)
project (covert-channels)
add_subdirectory (cc-libs)
add_subdirectory (channels/pp-l1d)
#add_subdirectory (channels/drng)
ExternalProject_Add(Flush-Reload
    GIT_REPOSITORY https://github.com/moehajj/Flush-Reload.git
    GIT_TAG "master"
    PREFIX ${PROJECT_SOURCE_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(Prime-Probe-L1d
    GIT_REPOSITORY https://github.com/yshalabi/cc-fun.git
    GIT_TAG "isca19-tut"
    PREFIX ${CMAKE_SOURCE_DIR}/external
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(mastik
    URL https://cs.adelaide.edu.au/~yval/Mastik/releases/Mastik-0.02-AyeAyeCapn.tgz
    PREFIX ${PROJECT_SOURCE_DIR}/external
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_COMMAND ""
)
