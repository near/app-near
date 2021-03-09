cmake_minimum_required(VERSION 3.10)
project(unittests C)

include(CTest)

set(CMAKE_C_STANDARD 11)

include_directories(../src)

add_executable(test_parser
        main.c
        ../src/parse_transaction.c)

target_compile_options(test_parser PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(test_parser PRIVATE UNITTEST)
target_link_libraries(test_parser PRIVATE cmocka)

add_test(test_parser test_parser)

if (FUZZ)
    if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Fuzzer needs to be built with Clang")
    endif()

    add_executable(fuzz_tx
        fuzz_tx.c
        ../src/parse_transaction.c)
    target_compile_options(fuzz_tx PRIVATE -fsanitize=address,fuzzer -g -ggdb2)
    target_compile_definitions(fuzz_tx PRIVATE UNITTEST)
    # target_link_options has been introduced in CMake 3.13, but Ubuntu 18.04 has CMake 3.10.2.
    # Use set_target_properties instead.
    set_target_properties(fuzz_tx PROPERTIES LINK_FLAGS "-fsanitize=address,fuzzer")
endif()
