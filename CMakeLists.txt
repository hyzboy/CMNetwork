cmake_minimum_required(VERSION 3.0)

project(CMNetwork)

include(path_config.cmake)
CMAudioSetup(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMNETWORK_ROOT_SOURCE_PATH})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Plug-Ins)

