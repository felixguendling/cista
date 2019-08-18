project(cista)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(cista-compile-flags
     -Wno-global-constructors
     -Wno-exit-time-destructors
     -fno-strict-aliasing
     -Weverything
     -Wno-c++98-compat
     -Wno-c++98-compat-pedantic
     -Wno-newline-eof
     -Wno-missing-prototypes
     -Wno-padded
     -Wno-double-promotion
     -Wno-undef
     -Wno-undefined-reinterpret-cast
     -Wno-float-conversion
     -Wno-return-std-move-in-c++11
     -Wno-gnu-zero-variadic-macro-arguments
     -Wno-unknown-pragmas
     -Werror
  )
endif()

option(CISTA_COVERAGE "generate coverage report" OFF)

add_library(cista INTERFACE)
target_include_directories(cista INTERFACE include)

add_subdirectory(tools/doctest EXCLUDE_FROM_ALL)

file(GLOB_RECURSE cista-include-files include/*.h*)

add_subdirectory(tools/uniter EXCLUDE_FROM_ALL)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cista.h
  COMMAND uniter
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/mmap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/serialization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/comparable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/printable.h
  > ${CMAKE_CURRENT_BINARY_DIR}/cista.h
  DEPENDS ${cista-include-files}
)

file(GLOB_RECURSE cista-test-files test/*.cc)
add_executable(cista-test-single-header EXCLUDE_FROM_ALL ${cista-test-files} ${CMAKE_CURRENT_BINARY_DIR}/cista.h)
target_link_libraries(cista-test-single-header doctest)
target_include_directories(cista-test-single-header PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(cista-test-single-header PRIVATE ${cista-compile-flags})
target_compile_definitions(cista-test-single-header PRIVATE SINGLE_HEADER)

add_executable(cista-test EXCLUDE_FROM_ALL ${cista-test-files})
target_compile_options(cista-test PRIVATE ${cista-compile-flags})
target_link_libraries(cista-test doctest cista)
if(CISTA_COVERAGE)
  target_compile_options(cista-test PRIVATE -fprofile-arcs -ftest-coverage)
  set_target_properties(cista-test PROPERTIES LINK_FLAGS --coverage)
endif()

add_custom_target(cista-format-check
  find
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    -type f
    (
    -name "*.cc"
    -o
    -name "*.h"
    -o
    -name "*.hpp"
    )
    -not -name "doctest.h"
    -print0
  | xargs -0 clang-format -i
  COMMAND
  git status --porcelain | xargs -I {} -0 test -z \"{}\"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking source code formatting"
  VERBATIM
)

add_custom_target(cista-lint
  /usr/bin/python
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-clang-tidy.py
      -exit-on-error
      -quiet
      -clang-tidy-binary clang-tidy-7
      -header-filter "^${CMAKE_CURRENT_SOURCE_DIR}/(include/)"
      "^${CMAKE_CURRENT_SOURCE_DIR}/test/"
  COMMENT "Running clang-tidy"
  VERBATIM
)
add_dependencies(cista-lint cista-test)

add_custom_target(cista-coverage
  rm -rf *.info &&
  find . -name "*.gcda" -delete &&
  ./cista-test &&
  lcov --directory . --capture --output-file cov.info &&
  lcov -r cov.info "*usr/include/*" -o cov.info &&
  lcov -r cov.info "*doctest*" -o cov.info &&
  lcov -r cov.info "*test/*.cc" -o cov.info &&
  lcov -r cov.info "*v1*" -o cov.info
)
add_dependencies(cista-coverage cista-test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_executable(cista-fuzz_graph EXCLUDE_FROM_ALL fuzz/fuzz_graph.cc)
  target_link_libraries(cista-fuzz_graph cista -fsanitize=address,undefined,fuzzer)
  target_compile_options(cista-fuzz_graph PRIVATE -g -O0 -fsanitize=address,fuzzer)
  
  add_executable(cista-fuzz_graph-seed EXCLUDE_FROM_ALL fuzz/fuzz_graph.cc)
  target_link_libraries(cista-fuzz_graph-seed cista)
  target_compile_definitions(cista-fuzz_graph-seed PRIVATE GENERATE_SEED)
endif()