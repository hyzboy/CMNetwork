cmake_minimum_required(VERSION 3.28)

project(CMNetwork)

include(path_config.cmake)
CMNetworkSetup(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMNETWORK_ROOT_SOURCE_PATH})


