cmake_minimum_required(VERSION 3.10)
project(unittests C)

include(CTest)

set(CMAKE_C_STANDARD 11)

add_executable(test_parser
        main.c
        ../src/parse_transaction.c)

target_include_directories(test_parser PRIVATE ../src)
target_compile_options(test_parser PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(test_parser PRIVATE UNITTEST)
target_link_libraries(test_parser PRIVATE cmocka)

add_test(test_parser test_parser)
