cmake_minimum_required(VERSION 3.8)

if (NOT DEFINED PROJECT_NAME AND CISTA_HASH STREQUAL "FNV1A")
  set(CISTA_INSTALL ON)
endif()

project(cista LANGUAGES CXX VERSION 0.7)

include(GNUInstallDirs)

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
     -Wno-documentation-unknown-command
     -Werror
  )
endif()

option(CISTA_ZERO_OUT "zero out fresh memory for valgrind" OFF)
option(CISTA_COVERAGE "generate coverage report" OFF)
option(CISTA_GENERATE_TO_TUPLE "generate include/cista/reflection/to_tuple.h" OFF)
set(CISTA_HASH "FNV1A" CACHE STRING "Options: FNV1A XXH3 WYHASH WYHASH_FASTEST")

add_library(cista INTERFACE)
if (CISTA_HASH STREQUAL "XXH3")
  add_subdirectory(tools/xxh3)
  target_link_libraries(cista INTERFACE xxh3)
elseif(CISTA_HASH STREQUAL "WYHASH" OR CISTA_HASH STREQUAL "WYHASH_FASTEST")
  add_subdirectory(tools/wyhash)
  target_link_libraries(cista INTERFACE wyhash)
endif()
target_compile_definitions(cista INTERFACE CISTA_${CISTA_HASH}=1)
if (CISTA_ZERO_OUT)
  target_compile_definitions(cista INTERFACE CISTA_ZERO_OUT=1)
endif()
target_include_directories(cista SYSTEM INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(cista INTERFACE cxx_std_17)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND
   ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" AND
   NOT "${CMAKE_CXX_FLAGS}" MATCHES "fsanitize")
  message(STATUS "Linking cista with mimalloc")
  target_link_libraries(cista INTERFACE mimalloc-static)
else()
  message(STATUS "Linking cista w/o mimalloc")
endif()

if (${CISTA_GENERATE_TO_TUPLE})
  add_subdirectory(tools/to_tuple_generator EXCLUDE_FROM_ALL)
  add_custom_target(generate_to_tuple
    COMMAND to_tuple_generator
      64  # max number of supported member fields
    > ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/to_tuple.h
  )
  add_dependencies(cista generate_to_tuple)
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cista/reflection/member_index.h
  > ${CMAKE_CURRENT_BINARY_DIR}/cista.h
  DEPENDS ${cista-include-files}
)

file(GLOB_RECURSE cista-test-files test/*.cc)
add_executable(cista-test-single-header EXCLUDE_FROM_ALL ${cista-test-files} ${CMAKE_CURRENT_BINARY_DIR}/cista.h)
target_link_libraries(cista-test-single-header cista-doctest)
target_include_directories(cista-test-single-header PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(cista-test-single-header PRIVATE ${cista-compile-flags})
target_compile_definitions(cista-test-single-header PRIVATE SINGLE_HEADER)
target_compile_features(cista-test-single-header PRIVATE cxx_std_17)
if (CISTA_ZERO_OUT)
  target_compile_definitions(cista-test-single-header PRIVATE CISTA_ZERO_OUT=1)
endif()

add_executable(cista-test EXCLUDE_FROM_ALL ${cista-test-files})
target_compile_options(cista-test PRIVATE ${cista-compile-flags})
target_link_libraries(cista-test cista-doctest cista)
if(CISTA_COVERAGE)
  target_compile_options(cista-test PRIVATE -fprofile-arcs -ftest-coverage)
  set_target_properties(cista-test PROPERTIES LINK_FLAGS --coverage)
endif()

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
  set(cista-fuzz-targets "")
  file(GLOB_RECURSE fuzz-files fuzz/*.cc)
  foreach(fuzz-file ${fuzz-files})
    get_filename_component(test-name ${fuzz-file} NAME_WE)

    add_executable(cista-fuzz-${test-name} EXCLUDE_FROM_ALL ${fuzz-file})
    target_link_libraries(cista-fuzz-${test-name} cista -fsanitize=address,undefined,fuzzer)
    target_compile_options(cista-fuzz-${test-name} PRIVATE -g -O0 -fsanitize=address,fuzzer)

    add_executable(cista-fuzz-${test-name}-seed EXCLUDE_FROM_ALL ${fuzz-file})
    target_link_libraries(cista-fuzz-${test-name}-seed cista)
    target_compile_definitions(cista-fuzz-${test-name}-seed PRIVATE GENERATE_SEED)

    add_custom_target(cista-fuzz-${test-name}-run
      DEPENDS
        cista-fuzz-${test-name}
        cista-fuzz-${test-name}-seed
      COMMAND
        mkdir -p fuzz-${test-name}-corpus &&
        ./cista-fuzz-${test-name}-seed ./fuzz-${test-name}-corpus/seed.bin &&
        ./cista-fuzz-${test-name} ./fuzz-${test-name}-corpus -max_total_time=120
    )

    list(APPEND cista-fuzz-targets cista-fuzz-${test-name}-run)
  endforeach()

  add_custom_target(cista-fuzz)
  add_dependencies(cista-fuzz ${cista-fuzz-targets})
endif()

add_library(cista::cista ALIAS cista)

# Export targets when not used via `add_subdirectory`
if (CISTA_INSTALL)
  include(CMakePackageConfigHelpers)
  set(CISTA_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cista")

  configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/CMake/cistaConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cistaConfig.cmake
    INSTALL_DESTINATION ${CISTA_CMAKE_CONFIG_DESTINATION}
  )

  install(
    TARGETS cista
    EXPORT cistaTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(
    EXPORT cistaTargets
    NAMESPACE cista::
    DESTINATION ${CISTA_CMAKE_CONFIG_DESTINATION}
  )

  install(
    DIRECTORY "include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cistaConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cistaConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/cistaConfigVersion.cmake"
    DESTINATION ${CISTA_CMAKE_CONFIG_DESTINATION}
  )
endif()
