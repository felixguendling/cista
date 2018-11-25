project(uniter)

add_executable(uniter uniter.cc)
target_compile_options(uniter PRIVATE "-std=c++17" "-Wall" "-Wextra")