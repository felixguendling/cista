project(doctest)

add_library(doctest STATIC doctest.cc)
target_include_directories(doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(doctest PUBLIC cxx_std_11)