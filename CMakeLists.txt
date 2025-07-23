cmake_minimum_required(VERSION 3.10)

project(small_mem_demo)

set(CMAKE_C_STANDARD 11)

find_package(small_mem REQUIRED)

add_executable(small_mem_demo
    main.c
)

target_link_libraries(small_mem_demo PRIVATE
    small_mem::small_mem
)
