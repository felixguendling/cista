project(cista)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cista INTERFACE)
target_include_directories(cista INTERFACE include)

add_subdirectory(tools/doctest EXCLUDE_FROM_ALL)

add_subdirectory(tools/uniter EXCLUDE_FROM_ALL)
add_custom_target(unite
  uniter
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/serialization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/comparable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/printable.h
  > ${CMAKE_CURRENT_BINARY_DIR}/cista.h
)
add_dependencies(unite uniter)

file(GLOB_RECURSE cista-test-files test/*.cc)
add_executable(cista-test EXCLUDE_FROM_ALL ${cista-test-files})
target_link_libraries(cista-test doctest)
target_include_directories(cista-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(cista-test unite)

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
      -clang-tidy-binary clang-tidy
      -header-filter "^${CMAKE_CURRENT_SOURCE_DIR}/(include/)"
      "^${CMAKE_CURRENT_SOURCE_DIR}/test/"
  COMMENT "Running clang-tidy"
  VERBATIM
)
add_dependencies(cista-lint cista-test)