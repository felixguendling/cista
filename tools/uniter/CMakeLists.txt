project(uniter)

add_executable(uniter uniter.cc)
target_compile_features(uniter PRIVATE cxx_std_17)