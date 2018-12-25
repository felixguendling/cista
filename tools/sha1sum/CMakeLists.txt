project(sha1)
cmake_minimum_required(VERSION 3.10)

add_library(sha1sum sha1sum.cc)
target_include_directories(sha1sum SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})