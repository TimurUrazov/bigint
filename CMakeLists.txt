cmake_minimum_required(VERSION 3.12)
project(bigint-task)

set(CMAKE_CXX_STANDARD 14)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "use msvcrt.dll in GTest" FORCE)
    set(gtest_extra_flags "-Dgtest_force_shared_crt=${gtest_force_shared_crt}")
endif()
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} "${gtest_extra_flags}" -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

add_subdirectory(
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL)

if (CYGWIN OR MINGW OR MSYS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
elseif (NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address,leak -fno-sanitize-recover=all -D_GLIBCXX_DEBUG")
endif()

add_executable(main
    big_integer.h
    big_integer.cpp
    tests.cpp)
target_link_libraries(main gtest_main)

if (ENABLE_SLOW_TEST)
    target_sources(main PRIVATE
        ci-extra/big_integer_gmp.h
        ci-extra/big_integer_gmp.cpp
        ci-extra/randomized_tests.cpp)

    target_link_libraries(main gmp)
endif()
