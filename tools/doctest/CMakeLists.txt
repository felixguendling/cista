project(doctest)

add_library(cista-doctest STATIC doctest.cc)
target_include_directories(cista-doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cista-doctest PUBLIC cxx_std_11)